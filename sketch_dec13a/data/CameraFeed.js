var gateway = `ws://${window.location.hostname}/ws`;
var socket;
var userPresent = true; // Assume the user is initially present

if (!('WebSocket' in window)) {
    alert('Your browser does not support web sockets');
} else {
    setupWebSocket();
}

function setupWebSocket() {
    var host = 'ws://' + window.location.hostname + ':81';
    socket = new WebSocket(host);
    socket.binaryType = 'arraybuffer';

    if (socket) {
        socket.onopen = function () {
            console.log('WebSocket connection opened');
        }

        socket.onmessage = function (msg) {
            var bytes = new Uint8Array(msg.data);
            var binary = '';
            var len = bytes.byteLength;

            for (var i = 0; i < len; i++) {
                binary += String.fromCharCode(bytes[i]);
            }

            var img = document.getElementById('live');
            img.src = 'data:image/jpg;base64,' + window.btoa(binary);
        }

        socket.onclose = function () {
            console.log('WebSocket connection closed');

        }

        // Track the last time the mouse moved
var lastMouseMoveTime = Date.now();

document.addEventListener('mousemove', function() {
    // Update the last mouse move time
    lastMouseMoveTime = Date.now();
});

function notifyUserPresence(present) {
    userPresent = present;
    if (socket && socket.readyState == WebSocket.OPEN) {
        socket.send(present ? "userPresent" : "userNotPresent");
    }
}

setInterval(function() {
    // Check if there has been mouse movement in the last minute
    var currentTime = Date.now();
    var userActive = currentTime - lastMouseMoveTime <= 15000;
    // Notify ESP32-CAM about user presence and PIR motion
    notifyUserPresence(userActive);

    }, 15000); // Adjust the interval based on your preference
    }
}

var isFlashlightOn = false; // This variable keeps track of the flashlight state

function toggleFlashlight() {
    // Reference the flashlight button using its ID
    var flashlightBtn = document.getElementById('flashlightButton');

    // Check the current state of the flashlight and change it
    isFlashlightOn = !isFlashlightOn;

    // Update the button text based on the flashlight state
    flashlightBtn.textContent = isFlashlightOn ? "ФЕНЕРЧЕ ВКЛ." : "ФЕНЕРЧЕ ИЗКЛ.";

    // Send the toggle command to the server via WebSocket
    if (socket && socket.readyState == WebSocket.OPEN) {
        socket.send("toggleFlashlight");
    }
}

function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();

    // Reload the page after capturing the photo
    setTimeout(function() {
        location.reload();
    }, 1000); // Adjust the delay based on your server processing time
}


function emailPhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/email-photo", true);
    xhr.send();
    // Trigger the custom alert
    document.getElementById('customAlert').style.display = "flex";
  
    // Start the countdown
    var countdown = 60; // Set the countdown time in seconds
    var alertCountdownElement = document.getElementById('alertCountdown');
    alertCountdownElement.textContent = countdown; // Display initial countdown time
    var countdownTimer = setInterval(function() {
        countdown -= 1;
        alertCountdownElement.textContent = countdown;
        if (countdown <= 0) {
            clearInterval(countdownTimer);
            document.getElementById('customAlert').style.display = "none";
            location.reload(); // Reload the page after countdown reaches zero
        }
    }, 1000);
}


function setupSlider(servoId) {
    var slider = document.getElementById(`servoSlider${servoId}`);
    slider.addEventListener('change', function(event) {
        var angle = event.target.value;
        updateServoAngle(servoId, angle);
    });
}

function updateServoAngle(servoId, angle) {
    // Update the displayed angle value
    document.getElementById(`servoPos${servoId}`).textContent = angle;

    // Send the servo angle to the ESP32-CAM server
    setServoAngle(servoId, angle);
}

function setServoAngle(servoId, angle) {
    // Create a new XMLHttpRequest object
    var xhr = new XMLHttpRequest();
    
    // Configure the request
    xhr.open("GET", `/set-servo-angle?servoId=${servoId}&angle=${angle}`, true);

    // Define the callback function to handle the server response
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                // Request was successful, handle response if needed
                console.log(xhr.responseText);
            } else {
                // Request failed, handle error if needed
                console.error("Failed to set servo angle");
            }
        }
    };

    // Send the GET request
    xhr.send();
}

// Call setupSlider for each servo
setupSlider(1); // For servo 1
setupSlider(2); // For servo 2

function updateClock() {
    var now = new Date();
    var hours = now.getHours();
    var minutes = now.getMinutes();
    var seconds = now.getSeconds();
        
        // Format time to have leading zeros where necessary
    hours = hours < 10 ? '0' + hours : hours;
    minutes = minutes < 10 ? '0' + minutes : minutes;
    seconds = seconds < 10 ? '0' + seconds : seconds;
    
    var timeString = hours + ':' + minutes + ':' + seconds;
        
        // Update the time display
    document.getElementById('time').textContent = timeString;
    }
    
    // Update the time every second
    setInterval(updateClock, 1000);
    
    // Initialize the clock on page load
    document.addEventListener('DOMContentLoaded', updateClock);

// Function to show the modal
// Function to show the modal
document.getElementById('editIcon').onclick = function() {
    // Check if the user can edit the title today
    if(canEditTitle()) {
        document.getElementById('editTitleModal').style.display = 'block';
    } else {
        alert("You've reached the limit of 5 changes for today.");
    }
};

// Function to apply the new title and save it to localStorage
function applyNewTitle() {
    var newTitle = document.getElementById('newTitleInput').value;
    if (newTitle) {
      document.getElementById('titleText').textContent = newTitle; // Update the title text
      localStorage.setItem('pageTitle', newTitle); // Save the title in localStorage
      document.getElementById('editTitleModal').style.display = 'none'; // Hide the modal
    } else {
      alert('Please enter a title.');
    }
  }
  
  // Function to load the saved title from localStorage
  function loadTitle() {
    var savedTitle = localStorage.getItem('pageTitle');
    if (savedTitle) {
      document.getElementById('titleText').textContent = savedTitle; // Set the title to the saved value
    }
  }
  
  // Call loadTitle on page load to set the custom title if one has been saved
  document.addEventListener('DOMContentLoaded', loadTitle);  

function canEditTitle() {
    const editInfo = JSON.parse(localStorage.getItem('editInfo')) || {};
    const today = new Date().toDateString();

    // Check if today's edits are less than 5
    if(editInfo.date === today && editInfo.count >= 10) {
        return false; // Limit reached
    }
    return true; // Can edit
}

function updateEditCount() {
    const today = new Date().toDateString();
    const editInfo = JSON.parse(localStorage.getItem('editInfo')) || {date: today, count: 0};

    if(editInfo.date === today) {
        editInfo.count += 1;
    } else {
        editInfo.date = today;
        editInfo.count = 1;
    }

    localStorage.setItem('editInfo', JSON.stringify(editInfo));
}

//Close modal when clicking outside of it
window.onclick = function(event) {
    var modal = document.getElementById('editTitleModal');
    if (event.target === modal) {
        modal.style.display = "none";
    }
};

function closeModal() {
    document.getElementById('editTitleModal').style.display = 'none';
  }

