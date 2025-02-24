#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP_Mail_Client.h>

// RFID Pins
#define SS_PIN 5   // GPIO 5 (D5)
#define RST_PIN 4  // GPIO 4 (D4)

// SMTP credentials
#define SMTP_HOST "smtp.gmail.com"   // Gmail SMTP server
#define SMTP_PORT 465                // SSL port for Gmail
#define AUTHOR_EMAIL "homemoniteringsystem@gmail.com"  // Sender email
#define AUTHOR_PASSWORD "dsivznmdtvpcazjp" // Sender email password
#define RECIPIENT_EMAIL "madetipavankumar9@gmail.com" // Recipient email
 
// Button & LED pins
#define BUTTON_PIN 13     // GPIO 13 (D4)
#define EMAIL_GREEN_LED 15  // GPIO 15 (D5)
#define EMAIL_RED_LED 2   // GPIO 2 (D18)
#define WIFI_GREEN_LED 12  // GPIO 21 (D21)
#define WIFI_RED_LED 22    // GPIO 22 (D22)

// Mail client object
SMTPSession smtp;
bool sendEmail(String ipAddress);
void indicateStatus(bool success);
// RFID object
MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi credentials
const char* ssid = "Pavan"; // Replace with your WiFi SSID
const char* password = "pavan123"; // Replace with your WiFi password

// Web server object
Filter your search...
Type:

All





AsyncWebServer server(80);

// Variable to store RFID card UID
String cardUID = "";

void setup() {
  Serial.begin(115200);

  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();

  // Set button and LED pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(EMAIL_GREEN_LED, OUTPUT);
  pinMode(EMAIL_RED_LED, OUTPUT);
  pinMode(WIFI_GREEN_LED, OUTPUT);
  pinMode(WIFI_RED_LED, OUTPUT);

  // Turn off LEDs initially
  digitalWrite(EMAIL_GREEN_LED, LOW);
  digitalWrite(EMAIL_RED_LED, LOW);
  digitalWrite(WIFI_GREEN_LED, LOW);
  digitalWrite(WIFI_RED_LED, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Try for 10 seconds
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    bool mailSent = sendEmail(WiFi.localIP().toString()); // Send email
    indicateStatus(mailSent); // Show status using LEDs

    // Turn ON Green LED for WiFi status
    digitalWrite(WIFI_GREEN_LED, HIGH);
    digitalWrite(WIFI_RED_LED, LOW);
  } else {
    Serial.println("\nFailed to connect to WiFi");

    // Turn ON Red LED for WiFi status
    digitalWrite(WIFI_GREEN_LED, LOW);
    digitalWrite(WIFI_RED_LED, HIGH);
  }

  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Pass Verification System</title>
  <!-- Bootstrap CSS -->
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/css/bootstrap.min.css" rel="stylesheet">
  <!-- Custom CSS -->
  <style>
    body {
      background-color: #f0f4f8;
      font-family: 'Arial', sans-serif;
    }
    .navbar {
      background-color: #004d00; /* APSRTC Green */
    }
    .navbar-brand, .navbar-nav .nav-link {
      color: #fff !important;
    }
    .navbar-brand:hover, .navbar-nav .nav-link:hover {
      color: #ffcb00 !important; /* Yellow hover effect */
    }
    .container {
      max-width: 800px;
      margin-top: 50px;
    }
    .table th, .table td {
      vertical-align: middle;
    }
    .alert {
      margin-top: 20px;
    }
    .profile-picture {
      max-width: 150px;
      border-radius: 10px;
    }
    .fraud-error {
      animation: shake 0.5s ease-in-out infinite;
    }
    @keyframes shake {
      0% { transform: translateX(0); }
      25% { transform: translateX(-5px); }
      50% { transform: translateX(5px); }
      75% { transform: translateX(-5px); }
      100% { transform: translateX(0); }
    }
    footer {
      background-color: #004d00;
      color: white;
      text-align: center;
      padding: 10px;
      margin-top: 30px;
    }
    footer a {
      color: #ffcb00;
    }
    .profile-container {
      display: flex;
      align-items: center;
      margin-bottom: 20px;
    }
    .profile-container .profile-picture {
      margin-right: 20px;
    }
    .from-to-row {
      display: flex;
      justify-content: space-between;
    }
  </style>
</head>
<body>
  <!-- Navbar -->
  <nav class="navbar navbar-expand-lg navbar-light">
    <div class="container-fluid">
      <a class="navbar-brand" href="#">APSRTC Pass Verification</a>
    </div>
  </nav>

  <!-- Main Content -->
  <div class="container">
    <h1 class="text-center mb-4">Pass Verification System</h1>
    <div class="card p-4 shadow">
      <div class="mb-3">
        <label for="phoneInput" class="form-label">Enter Student Phone Number:</label>
        <input type="text" class="form-control" id="phoneInput" placeholder="Enter phone number">
      </div>
      <button id="verifyButton" class="btn btn-success w-100">Verify Pass</button>
    </div>

    <!-- Details Section -->
    <div id="detailsSection" class="mt-4 d-none">
      <h2 class="text-center mb-4">Student Details</h2>
      <div class="profile-container">
        <img id="profilePicture" src="" alt="Profile Picture" class="profile-picture">
        <div>
          <div id="passStatus" class="alert"></div>
          <div class="from-to-row">
            <div><strong>From:</strong> <span id="fromField"></span></div>
            <div><strong>To:</strong> <span id="toField"></span></div>
          </div>
        </div>
      </div>
      <div class="table-responsive">
        <table class="table table-bordered">
          <thead>
            <tr>
              <th>Field</th>
              <th>Details</th>
            </tr>
          </thead>
          <tbody id="detailsTable">
            <!-- Details will be populated here -->
          </tbody>
        </table>
      </div>
      <button id="blockPassButton" class="btn btn-danger w-100 mt-4">Block Pass</button>
    </div>
  </div>

  <!-- Footer -->
  <footer>
    <p>&copy; 2025 APSRTC. All rights reserved.</p>
  </footer>

  <!-- Fraud Error Modal -->
  <div class="modal fade" id="fraudErrorModal" tabindex="-1" aria-labelledby="fraudErrorModalLabel" aria-hidden="true">
    <div class="modal-dialog">
      <div class="modal-content">
        <div class="modal-header">
          <h5 class="modal-title" id="fraudErrorModalLabel">Fraud Alert</h5>
          <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
        </div>
        <div class="modal-body">
          <p>Your pass has a fraud report. Please contact the nearby APSRTC BUS DEPOT.</p>
        </div>
        <div class="modal-footer">
          <button type="button" class="btn btn-danger" data-bs-dismiss="modal">Close</button>
        </div>
      </div>
    </div>
  </div>

  <!-- Script to fetch UID from ESP32 -->
  <script>
    setInterval(async () => {
      const response = await fetch('/uid');
      const uid = await response.text();
      if (uid) {
        document.getElementById('phoneInput').value = uid;
      }
    }, 1000);
  </script>
  <script type="module">
    import { initializeApp } from "https://www.gstatic.com/firebasejs/9.6.11/firebase-app.js";
    import { getDatabase, ref, get, update } from "https://www.gstatic.com/firebasejs/9.6.11/firebase-database.js";

    const firebaseConfig = {
      apiKey: "AIzaSyA02GfjPGm1X8He9bLkrfkyBKWYgmMc98Q",
      authDomain: "buspass-2025.firebaseapp.com",
      projectId: "buspass-2025",
      storageBucket: "buspass-2025.appspot.com",
      messagingSenderId: "417197327685",
      appId: "1:417197327685:web:42d2d9e773512f7ba1ea2c",
      measurementId: "G-475C145WWK"
    };

    const app = initializeApp(firebaseConfig);
    const database = getDatabase(app);

    const phoneInput = document.getElementById("phoneInput");
    const verifyButton = document.getElementById("verifyButton");
    const detailsSection = document.getElementById("detailsSection");
    const detailsTable = document.getElementById("detailsTable");
    const profilePicture = document.getElementById("profilePicture");
    const passStatus = document.getElementById("passStatus");
    const blockPassButton = document.getElementById("blockPassButton");
    const fromField = document.getElementById("fromField");
    const toField = document.getElementById("toField");

    const fraudErrorModal = new bootstrap.Modal(document.getElementById('fraudErrorModal'));

    let currentPhone = "";

    verifyButton.addEventListener("click", async () => {
      const phone = phoneInput.value.trim();
      if (!phone) {
        alert("Please enter a valid phone number.");
        return;
      }

      currentPhone = phone;

      const studentRef = ref(database, `busPasses/${phone}`);
      const snapshot = await get(studentRef);

      if (snapshot.exists()) {
        const studentData = snapshot.val();

        // Debugging: Log fraud status
        console.log("Fraud Status: ", studentData.fraud);

        // Check fraud status
        if (studentData.fraud === "yes") {
          // Show fraud error in the modal
          fraudErrorModal.show();
          detailsSection.classList.add("d-none"); // Hide details section
          return; // Stop further execution if fraud is detected
        }

        // Display details
        detailsTable.innerHTML = `
          <tr><td>Name</td><td>${studentData.name}</td></tr>
          <tr><td>Age</td><td>${studentData.age}</td></tr>
          <tr><td>Expiry Date</td><td><p style="color:red; font-weight: bolder;">${studentData.expiryDate}</p></td></tr>
          <tr><td>College</td><td>${studentData.college}</td></tr>
          <tr><td>Place</td><td>${studentData.place}</td></tr>
          <tr><td>Date of Birth</td><td>${studentData.dob}</td></tr>
          <tr><td>Phone</td><td>${studentData.phone}</td></tr>
          <tr><td>Email</td><td>${studentData.email}</td></tr>
          <tr><td>Address</td><td>${studentData.address}</td></tr>
          <tr><td>Pincode</td><td>${studentData.pincode}</td></tr>
          <tr><td>State</td><td>${studentData.state}</td></tr>
          <tr><td>Validity</td><td>${studentData.validity}</td></tr>
          <tr><td>Price</td><td>${studentData.price} INR</td></tr>
        `;

        // Display profile picture
        profilePicture.src = studentData.profilePicture;

        // Set From and To values
        fromField.innerHTML = studentData.from;
        toField.innerHTML = studentData.to;

        // Check expiry date
        const currentDate = new Date();
        const expiryDate = new Date(studentData.expiryDate);

        if (expiryDate < currentDate) {
          passStatus.innerHTML = `<div class="alert alert-danger">Pass Expired!</div>`;
        } else {
          passStatus.innerHTML = `<div class="alert alert-success">Pass Verified Successfully!</div>`;
        }

        // Show details section
        detailsSection.classList.remove("d-none");
      } else {
        alert("No student found with this phone number.");
        detailsSection.classList.add("d-none");
      }
    });

    // Block pass button click event
    blockPassButton.addEventListener("click", async () => {
      const studentRef = ref(database, `busPasses/${currentPhone}`);
      const snapshot = await get(studentRef);

      if (snapshot.exists()) {
        const studentData = snapshot.val();

        // Update fraud status to "yes"
        await update(studentRef, {
          fraud: "yes"
        });

        // Show success message
        alert("Pass has been blocked successfully.");
        detailsSection.classList.add("d-none");
      } else {
        alert("Student not found.");
      }
    });
  </script>

  <!-- Bootstrap JS -->
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // Endpoint to fetch UID
  server.on("/uid", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", cardUID);
  });

  server.begin();
}

void loop() {
  // Check for new RFID card
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    cardUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      cardUID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      cardUID += String(rfid.uid.uidByte[i], HEX);
    }
    Serial.println(cardUID);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  //Check if button is pressed
  // if (digitalRead(BUTTON_PIN) == HIGH) {
  //   delay(200); // Debounce delay
  //   if (digitalRead(BUTTON_PIN) == HIGH) {
  //     Serial.println("Button pressed!");
  //     bool mailSent = sendEmail(WiFi.localIP().toString()); // Send email
  //     indicateStatus(mailSent); // Show status using LEDs
  //     while (digitalRead(BUTTON_PIN) == HIGH); // Wait for button release
  //   }
  // }
}

bool sendEmail(String ipAddress) {
  smtp.debug(1);

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name = "ESP32";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP32 IP Address";
  message.addRecipient("Pavan Kumar", RECIPIENT_EMAIL);
  message.text.content = "The ESP32's IP address is: " + ipAddress;
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session)) {
    Serial.println("Mail connection failed!");
    return false;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Email failed: " + smtp.errorReason());
    return false;
  }

  Serial.println("Email sent successfully!");
  return true;
}

void indicateStatus(bool success) {
  if (success) {
    digitalWrite(EMAIL_GREEN_LED, HIGH);
    delay(5000);
    digitalWrite(EMAIL_GREEN_LED, LOW);
  } else {
    digitalWrite(EMAIL_RED_LED, HIGH);
    delay(5000);
    digitalWrite(EMAIL_RED_LED, LOW);
  }
}
