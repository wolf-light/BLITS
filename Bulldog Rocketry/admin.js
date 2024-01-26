// Firebase configuration
const firebaseConfig = {
    apiKey: "AIzaSyDc1bptlOncgHD3i-c0Br8lzMKYGnM_0Ys",
    authDomain: "bulldog-rocketry-a06cc.firebaseapp.com",
    projectId: "bulldog-rocketry-a06cc",
    storageBucket: "bulldog-rocketry-a06cc.appspot.com",
    messagingSenderId: "313636010269",
    appId: "1:313636010269:web:d1d79f52fdd5b6e037c9f4",
    measurementId: "G-G4TY444NQQ"
};

// Initialize Firebase
const app = firebase.initializeApp(firebaseConfig);
const auth = app.auth();
const database = firebase.database();
var uid;

function getCookie(cname) {
    let name = cname + "=";
    let decodedCookie = decodeURIComponent(document.cookie);
    let ca = decodedCookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) == ' ') {
            c = c.substring(1);
        }
        if (c.indexOf(name) == 0) {
            return c.substring(name.length, c.length);
        }
    }
    return "";
}

function checkRequiredCookies() {
    const emailCookie = getCookie("email");
    const domainCookie = getCookie("domain");
    const uidCookie = getCookie("uid");

    if (!emailCookie || !domainCookie || !uidCookie) {
        // One or more required cookies are missing, redirect to the 404 page
        window.location.href = "404.html";
    }
}


document.addEventListener('DOMContentLoaded', function () {
    checkRequiredCookies();
    // Other initialization code for the admin page goes here...
});

// Function to fetch all users from the database and populate the table
function fetchAllUsers() {
    const usersRef = database.ref('Users');
    usersRef.once('value', function (snapshot) {
        console.log('Snapshot:', snapshot.val());
        const userTable = document.getElementById('userTable');
        userTable.innerHTML = `
        <tr>
          <th>UID</th>
          <th>Email</th>
          <th>Name</th>
          <th>Auth Value</th>
          <th>Actions</th>
        </tr>
      `;

        snapshot.forEach(function (childSnapshot) {
            uid = childSnapshot.key;
            const userData = childSnapshot.val();
            const { email, name, auth } = userData;
            const row = document.createElement('tr');
            row.innerHTML = `
          <td>***</td>
          <td>***</td>
          <td>${name}</td>
          <td>
          <select id="authSelect_${uid}" ${auth === 4 ? 'disabled' : ''}>
              <option value="0" ${auth === 0 ? 'selected' : ''}>0 (Deactivated)</option>
              <option value="1" ${auth === 1 ? 'selected' : ''}>1 (External)</option>
              <option value="2" ${auth === 2 ? 'selected' : ''}>2 (Internal)</option>
              <option value="3" ${auth === 3 ? 'selected' : ''}>3 (Both)</option>
              <option value="4" ${auth === 4 ? 'selected' : ''}>4 (ADMIN)</option>
              <!-- Add more options as needed -->
            </select>
          </td>
          <td>
            <button onclick="updateAuthValue('${uid}')">Save</button>
          </td>
        `;
            userTable.appendChild(row);
        });
    });
}

// Function to update the authValue for a specific user
function updateAuthValue(uid) {
    const authSelect = document.getElementById(`authSelect_${uid}`);
    const newAuthValue = parseInt(authSelect.value);
    const userRef = database.ref(`Users/${uid}`);
    userRef.update({ auth: newAuthValue })
        .then(function () {
            console.log(`AuthValue updated for UID ${uid}`);
        })
        .catch(function (error) {
            console.error('Error updating authValue:', error);
        });
}

// Listen for the DOM content to load and then fetch users
document.addEventListener('DOMContentLoaded', function () {
    fetchAllUsers();
});
