const express = require('express');
const { exec, execFile } = require('child_process');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');

const { parse } = require('csv-parse'); // This is for parsing raw CSV
const fastcsv = require('fast-csv'); // This is for reading/writing CSV files

const app = express();
const PORT = 3000;

// Middleware
app.use(bodyParser.urlencoded({ extended: false }));
app.use(express.raw({ type: 'text/csv', limit: '1mb' })); // Add raw body parser for CSV data
app.use(express.static('public'));
app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// Use the specified directory for chat messages
const chatDir = path.join(__dirname, 'chatbox');
if (!fs.existsSync(chatDir)) {
  fs.mkdirSync(chatDir, { recursive: true });
}

// Chat message file paths - single CSV file for both mentor and mentee
const mentorMessagesPath = path.join(__dirname, "chatbox/mentor_messages.csv");
const menteeMessagesPath = path.join(__dirname, "chatbox/mentee_messages.csv");
const messagesFilePath = path.join(__dirname, "chatbox/messages.csv");

// Ensure CSV files exist
if (!fs.existsSync(menteeMessagesPath)) {
  fs.writeFileSync(menteeMessagesPath, '');
}
if (!fs.existsSync(mentorMessagesPath)) {
  fs.writeFileSync(mentorMessagesPath, '');
}

// Start with identity selection
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/public/identity.html');
});

// Store userType in URL params and redirect
app.post('/select-identity', (req, res) => {
  const { identity } = req.body;
  if (identity === 'mentor' || identity === 'mentee') {
    res.redirect(`/login.html?userType=${identity}`); // Pass userType in URL
  } else {
    res.status(400).send('Invalid identity selection');
  }
});

// Handle login (userType comes from hidden form field)
app.post('/login', (req, res) => {
  const { username, password, userType } = req.body; // Now from form, not cookie
  execFile(path.join(__dirname, 'login', 'login.exe'), ['login', username, password, userType], (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);

    if (error || !stdout.includes('[SUCCESS]')) {
      return res.status(401).send('Login failed. Invalid credentials.');
    }

    if (userType === 'mentor') {
      return res.redirect('mentor/mentor-home.html');
    } else if (userType === 'mentee') {
      return res.redirect('mentee/mentee-home.html');
    } else {
      return res.status(400).send('Unknown user type.');
    }

  });
});

// Handle registration (userType comes from hidden form field)
app.post('/register', (req, res) => {
  const { username, password, userType } = req.body;
  console.log('EXEC:', username, password, userType);
  execFile(path.join(__dirname, 'login', 'login.exe'), ['register', username, password, userType], (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);
    
    if (error || !stdout.includes('[SUCCESS]')) {
      return res.status(401).send('Registration failed.');
    }

    if (userType === 'mentor') {
      return res.redirect('/mentor/mentor-home.html');
    } else if (userType === 'mentee') {
      return res.redirect('/mentee/mentee-home.html');
    } else {
      return res.status(400).send('Unknown user type.');
    }

  });
});

app.get('/load-meetings', (req, res) => {
    execFile(path.join(__dirname, 'meetings', 'loadMeetings.exe'), (error, stdout, stderr) => { 
        if (error) {
            console.error(`Error running C++ executable:`, error);
            return res.status(500).send('Internal server error');
        }

        const slots = stdout.split('\n').filter(line => line.length > 0).map(line => {
            const [slot, day, month, year] = line.split(',');
            return { slot, day: Number(day), month: Number(month), year: Number(year) };
        });

        res.json(slots); // Send available slots as JSON to frontend
    });
});

app.get('/load-available-meetings', (req, res) => {
  execFile(path.join(__dirname, 'meetings', 'loadAvailableMeetings.exe'), (error, stdout, stderr) => { 
      if (error) {
          console.error(`Error running C++ executable:`, error);
          return res.status(500).json({ error: 'Internal server error' });
      }

      const slots = stdout.split('\n')
          .filter(line => line.trim().length > 0)
          .map(line => {
              const [slot, day, month, year] = line.split(',');
              return { 
                  slot: slot.trim(), 
                  day: parseInt(day.trim()), 
                  month: parseInt(month.trim()), 
                  year: parseInt(year.trim()) 
              };
          });

      res.json(slots);
  });
});

app.post('/set-availability', (req, res) => {
  const { name = null, day, slot } = req.body; // Default to null instead of "Anonymous"

  // Parse date
  const [year, month, dayOfMonth] = day.split('-').map(Number);
  const slots = Array.isArray(slot) ? slot : [slot];

  const results = [];
  const errors = [];

  const promises = slots.map((s) => {
    let hour;
    if (s === 'A') hour = 16;
    else if (s === 'B') hour = 17;
    else if (s === 'C') hour = 18;
    else {
      errors.push(`Invalid time slot: ${s}`);
      return Promise.resolve(); // Skip invalid slots
    }

    return new Promise((resolve, reject) => {
      console.log(`Calling C program with: name=null, slot=${s}, day=${dayOfMonth}, month=${month}, year=${year}, booked=0`);

      execFile(path.join(__dirname, 'meetings', 'meetings.exe'), 
        [null, s, dayOfMonth, month, year, '0'], // Pass null as name
        (error, stdout, stderr) => {
          if (error || !stdout.includes('[SUCCESS]')) {
            errors.push(`Slot ${s} failed: ${stderr || stdout}`);
            reject();
          } else {
            results.push(`Slot ${s} saved`);
            resolve();
          }
        }
      );
    });
  });

  Promise.allSettled(promises).then(() => {
    if (errors.length > 0) {
      return res.status(207).json({ // 207 Multi-Status
        success: false,
        message: 'Some slots failed to submit',
        errors,
        savedSlots: results
      });
    }
    res.json({
      success: true,
      message: 'Availability set for all slots',
      savedSlots: results
    });
  });
});

app.post('/book-meeting', (req, res) => {
  const { name, slot, day, month, year } = req.body; // Already separated
  
  // No need to split - use the values directly
  execFile(path.join(__dirname, 'meetings', 'meetings.exe'), 
    [name, slot, day, month, year, '1'], 
    (error, stdout, stderr) => {
      
      if (error) {
        console.error('Booking failed:', error || stderr);
        return res.status(500).json({ error: 'Error saving booking' });
      }
      res.json({ success: true, message: 'Booking Saved!' });
  });
});


// Tasks
const tasks = path.join(__dirname, 'tasks', 'tasks.exe');

function executeHeapCommand(menteeName, command, task = '') {
  return new Promise((resolve, reject) => {
      let cmd = `"${tasks}" "${menteeName}" ${command}`;
      
      if (command === 'insert' && task) {
          const { title, description, dueDate, priority } = task;
          const escapedTitle = title.replace(/"/g, '\\"');
          const escapedDescription = description.replace(/"/g, '\\"');
          cmd += ` "${escapedTitle}" "${escapedDescription}" "${dueDate}" "${priority}"`;
      }

      if (command === 'removeSpecific' && typeof task === 'string') {
          const escapedTitle = task.replace(/"/g, '\\"');
          cmd += ` "${escapedTitle}"`;
      }

      exec(cmd, (error, stdout, stderr) => {
          if (error) {
              console.error(`Error executing command: ${error.message}`);
              return reject(error);
          }
          if (stderr) {
              console.error(`Command stderr: ${stderr}`);
              return reject(new Error(stderr));
          }
          resolve(stdout);
      });
  });
}


app.post('/add-task', async (req, res) => {
  try {
      const { menteeName, taskTitle, taskDescription, taskDueDate, taskPriority } = req.body;
      if (!menteeName || !taskTitle || !taskDescription || !taskDueDate || !taskPriority) {
          return res.status(400).send('All fields including menteeName are required.');
      }

      await executeHeapCommand(menteeName, 'insert', {
          title: taskTitle,
          description: taskDescription,
          dueDate: taskDueDate,
          priority: taskPriority
      });

      res.send('Task added successfully!');
  } catch (error) {
      res.status(500).send('Failed to add task. Please try again.');
  }
});


app.post('/remove-task', async (req, res) => {
  try {
      const { menteeName, title } = req.body;
      if (!menteeName || !title) return res.status(400).send('menteeName and task title are required');

      const result = await executeHeapCommand(menteeName, 'removeSpecific', title);
      res.send(result);
  } catch (error) {
      res.status(500).send('Failed to remove task.');
  }
});


app.post('/clear-tasks', async (req, res) => {
  try {
      const { menteeName } = req.body;
      if (!menteeName) return res.status(400).send('menteeName is required');

      await executeHeapCommand(menteeName, 'clear');
      res.send('All tasks cleared from the heap successfully!');
  } catch (error) {
      res.status(500).send('Failed to clear tasks.');
  }
});


app.get('/list-tasks', async (req, res) => {
  try {
      const menteeName = req.query.menteeName;
      if (!menteeName) return res.status(400).send('menteeName is required');

      const result = await executeHeapCommand(menteeName, 'list');
      const tasks = result.trim().split('\n').map(line => {
        const [mentee, title, description, dueDate, priority] = line.split('|').map(s => s.trim());
          return { mentee, title, description, dueDate, priority };
      });
      res.json(tasks);
  } catch (error) {
      res.status(500).send('Failed to load tasks.');
  }
});

//Tasks End

//menteeslist begins

//insert
app.post('/insert-mentees', (req, res) => {
  const { roll_no, name, department } = req.body;

  // Use absolute path for safety
  const exePath = path.join(__dirname, 'menteeslist', 'menteeslist.exe');
  const command = `"${exePath}" insert ${roll_no} "${name}" "${department}"`;

  exec(command, (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);

    if (error) {
      return res.status(500).send(`Error: ${stderr}`);
    }

    res.send(stdout || 'Student inserted successfully.');
  });
});


// Endpoint to delete a mentee
app.post('/delete-mentees', (req, res) => {
  const { roll_no } = req.body;

  // Use absolute path for safety
  const exePath = path.join(__dirname, 'menteeslist', 'menteeslist.exe');

  const command = `"${exePath}" delete ${roll_no}`;

  exec(command, (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);

    if (error) {
      return res.status(500).send(`Error: ${stderr}`);
    }

    res.send(stdout || 'Student deleted successfully.');
  });
});

// Endpoint to search for a mentee
app.post('/search-mentees', (req, res) => {
  const { roll_no } = req.body;

  // Use absolute path for safety
  const exePath = path.join(__dirname, 'menteeslist', 'menteeslist.exe');

  const command = `"${exePath}" search ${roll_no}`;

  exec(command, (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);

    if (error) {
      return res.status(500).send(`Error: ${stderr}`);
    }

    res.send(stdout || 'Student not found.');
  });
});

// Endpoint to load and list all mentees
app.get('/load-mentees', (req, res) => {

  const exePath = path.join(__dirname, 'menteeslist', 'menteeslist.exe');
  const command = `"${exePath}" load`;
  
  exec(command, (error, stdout, stderr) => {
    console.log('stdout:', JSON.stringify(stdout));
    console.log('stderr:', JSON.stringify(stderr));
    console.log('error:', error);

    if (error) {
      return res.status(500).send(`Error loading mentees: ${stderr}`);
    }

    res.send(stdout || 'No mentees found.');
  });
});
//menteeslist ends

// ===== CHAT FUNCTIONALITY BEGIN =====

// Create messages.csv if it doesn't exist
if (!fs.existsSync(messagesFilePath)) {
  fs.writeFileSync(messagesFilePath, '');
}

// Root route to serve chatbox.html as the homepage
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'chatbox.html')); // Send chatbox.html as the homepage
});

app.get('/getMessages', (req, res) => {
    let messagesHTML = '';

    fs.createReadStream(messagesFilePath)
        .pipe(fastcsv.parse({ headers: false, skipEmptyLines: true })) // Use fast-csv to parse the CSV
        .on('data', (row) => {
            const text = row[0]; // Access the message text
            const sender = row[1]; // Access the sender
            messagesHTML += `<div class="message ${sender}">${text}</div>`; // Construct message HTML
        })
        .on('end', () => {
            res.send(messagesHTML); // Send the constructed HTML as the response
        })
        .on('error', (err) => {
            console.error("Error reading CSV:", err);
            res.status(500).send("Error reading messages.");
        });
});

// Write a new message to the CSV file
app.post('/sendMessage', (req, res) => {
    const csvData = req.body.toString(); // Convert the raw buffer data to a string

    // Parse the CSV data using csv-parse
    parse(csvData, { columns: false, trim: true }, (err, records) => {
        if (err) {
            console.error("Error parsing CSV:", err);
            return res.status(400).send("Invalid CSV data.");
        }

        // Assuming each message is one record with "text, sender"
        const [text, sender] = records[0]; // Extract text and sender from the CSV

        if (!text || !sender) {
            return res.status(400).send("Missing message data.");
        }

        // Append the message to the CSV file
        const writeStream = fs.createWriteStream(messagesFilePath, { flags: 'a' });
        const messageRow = `"${text}","${sender}"\n`; // Format as CSV row
        writeStream.write(messageRow);
        writeStream.end();

        res.status(200).send("Message saved.");
    });
});
// ===== CHAT FUNCTIONALITY END =====


app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});