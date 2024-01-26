var verifiedUser = 0;
const userInfo = []
const allowedDomain = ['d.umn.edu', 'umn.edu'];

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
firebase.analytics();

// Function to get the value of a cookie by its name
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

// Function to set the value of a cookie
function setCookie(cname, cvalue) {
    var d = new Date();
    d.setTime(d.getTime() + (365 * 24 * 60 * 60 * 1000)); // Set cookie expiration to 1 year
    var expires = "expires=" + d.toUTCString();
    document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

// Check if the cookie exists and populate the userInfo object
function checkCookie() {
    const emailCookie = getCookie("email");
    const domainCookie = getCookie("domain");
    const uidCookie = getCookie("uid");

    if (emailCookie && domainCookie && uidCookie) {
        userInfo.email = emailCookie;
        userInfo.domain = domainCookie;
        userInfo.uid = uidCookie;
        console.log("User info retrieved from cookie:", userInfo);
    } else {
        console.log("No user info found in cookie");
    }
}

// Save the userInfo object to a cookie
function saveUserInfoToCookie() {
    setCookie("email", userInfo.email);
    setCookie("domain", userInfo.domain);
    setCookie("uid", userInfo.uid);
    console.log("User info saved to cookie:", userInfo);
}

window.addEventListener("load", function () {
    checkCookie();
    // Move the call to beginMainProcess() inside the checkCookie() function
    beginMainProcess();
});

function Check() {
    if (allowedDomain.includes(userInfo.domain)) {
        var uid = userInfo.uid;
        var userRef = firebase.database().ref('Users/' + userInfo.uid);

        userRef.child('auth').once('value', function (snapshot) {
            userInfo.authValue = snapshot.val();

            isUserAuthorized(userInfo, function (isAuthorized) {
                // Domain is allowed and auth value is 1
                showLinkForm();
                beginMainProcess();
            });
            // else {
            //     // Domain is allowed but auth value is not 1
            //     hideLinkForm();
            //     window.alert("Account needs authorization.");
            // }
        });
    } else {
        // Domain is not allowed
        hideLinkForm();
    }
}


function showLinkForm() {
    var linkFormContainer = document.getElementById('linkFormContainer');
    linkFormContainer.style.display = 'block';
    var authContainer = document.getElementById('firebaseui-auth-container');
    authContainer.style.display = 'none';
}

function hideLinkForm() {
    var linkFormContainer = document.getElementById('linkFormContainer');
    linkFormContainer.style.display = 'none';
    var authContainer = document.getElementById('firebaseui-auth-container');
    authContainer.style.display = 'block';
}

function signIn() {

}

var uiConfig = {
    signInOptions: [
        firebase.auth.EmailAuthProvider.PROVIDER_ID
    ],
    callbacks: {
        signInSuccessWithAuthResult: function (authResult, redirectUrl = false) {
            var user = authResult.user;
            var email = user.email;
            var uid = user.uid;
            var domain = email.split('@')[1];
            userInfo.email = email;
            userInfo.domain = domain;
            userInfo.uid = uid;
            console.log(email);
            console.log(domain);
            console.log(uid);
            if (allowedDomain.includes(userInfo.domain)) {
                // Domain is allowed, continue with sign-in process
                console.log('User signed in:', user);
                saveUserInfoToCookie(); // Save userInfo to cookie
                Check();
                return false; // Allow sign-in
            } else {
                // Domain is not allowed
                console.log('User email domain not allowed');
                return false; // Prevent sign-in
            }
        }
    }
};

firebase.auth().onAuthStateChanged(function (user) {
    if (user) {
        // User is signed in, check if their UID exists in the "Users" node
        const userRef = firebase.database().ref(`Users/${user.uid}`);
        userRef.once('value', function (snapshot) {
            if (!snapshot.exists()) {
                // UID does not exist in the "Users" node, ask for their name
                const name = window.prompt("Please enter your name:");

                // Add the user information to the database
                userRef.set({
                    email: user.email,
                    name: name || 'Unknown', // Use 'Unknown' if the name is not provided
                    auth: 0 // Set the default auth value, you can change it as needed
                })
                    .then(function () {
                        console.log('User UID added to "Users" node:', user.uid);
                    })
                    .catch(function (error) {
                        console.error('Error adding user UID to "Users" node:', error);
                    });
            }
        });
    }
});






function signOut() {
    console.log("Sign Out");
    // Clear user-related cookies
    setCookie("email", "");
    setCookie("domain", "");
    setCookie("uid", "");

    firebase
        .auth()
        .signOut()
        .then(() => {
            // Sign-out successful. Refresh the page to update the UI.
            location.reload();
        })
        .catch((error) => {
            // An error happened. Log the error to the console.
            console.error("Error signing out:", error);
        });
}

// Function to set the value of a cookie
function setCookie(cname, cvalue) {
    var d = new Date();
    d.setTime(d.getTime() + (365 * 24 * 60 * 60 * 1000)); // Set cookie expiration to 1 year
    var expires = "expires=" + d.toUTCString();
    document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}


// Initialize the FirebaseUI Widget using Firebase.
var ui = new firebaseui.auth.AuthUI(firebase.auth());
// The start method will wait until the DOM is loaded.
ui.start('#firebaseui-auth-container', uiConfig);


//============================================================================================
//======================= END AUTHENTICATION // BEGIN FORM ===================================
//============================================================================================

var nodeRef = 'links';
// Reference to your Firebase database
var database = firebase.database().ref(nodeRef);



function addLink(title, url, color, order) {
    isUserAuthorized(userInfo, function (isAuthorized) {
        const nodeSelection = document.getElementById('nodeSelection').value;
        const nodeRef = (nodeSelection === 'links2' || nodeSelection === 'podcast') ? nodeSelection : 'links';
        if (title) {
            const linkId = `link${order}`; // Generate the linkId based on the order
            firebase.database().ref(nodeRef).child(linkId).set({
                title: title,
                url: url,
                color: color,
                order: order
            });
        }
    });
}



function updateLink(linkId, title, url, color) {
    isUserAuthorized(userInfo, function (isAuthorized) {
        const nodeSelection = document.getElementById('nodeSelection').value;
        const nodeRef = (nodeSelection === 'links2' || nodeSelection === 'podcast') ? nodeSelection : 'links';

        firebase.database().ref(nodeRef).child(linkId).update({
            title: title,
            url: url,
            color: color
        });
    });
}


function deleteLink(linkId) {
    isUserAuthorized(userInfo, function (isAuthorized) {
        const nodeSelection = document.getElementById('nodeSelection').value;
        const nodeRef = (nodeSelection === 'links2' || nodeSelection === 'podcast') ? nodeSelection : 'links';

        if (confirm('Are you sure you want to delete this link?')) {
            firebase.database().ref(nodeRef).child(linkId).remove();
        }
    });
}


function beginMainProcess() {
    var nodeSelection = document.getElementById('nodeSelection').value;
    const nodeRef = (nodeSelection === 'links2' || nodeSelection === 'podcast') ? nodeSelection : 'links';
    var database = firebase.database().ref(nodeRef);

    // Move the rest of the code inside the callback function of orderByChild()
    isUserAuthorized(userInfo, function (isAuthorized) {
        // Form submission event listener
        var linkForm = document.getElementById('linkForm');
        if (linkForm) {
            linkForm.addEventListener('submit', function (e) {
                e.preventDefault(); // Prevent form submission

                // Get form values
                var title = document.getElementById('title').value;
                var url = document.getElementById('url').value;
                var color = document.getElementById('color').value;
                var order = document.getElementById('order').value;

                // Check if it's an update or add operation
                var linkId = linkForm.dataset.linkId;
                if (linkId) {
                    // Update existing link
                    updateLink(linkId, title, url, color);
                } else {
                    // Add new link
                    addLink(title, url, color, order);
                }

                // Reset the form
                linkForm.reset();
                document.getElementById('order').disabled = false;
                document.getElementById('order').classList.remove('not-allowed');
                linkForm.removeAttribute('data-link-id');
                document.getElementById('addLinkBtn').style.display = 'block';
                document.getElementById('updateLinkBtn').style.display = 'none';
            });
        } else {
            window.alert("BAD ERROR!!! VERY BAD!!!");
        }


        isUserAuthorized(userInfo, function (isAuthorized) {
            // Retrieve and display existing links in the specified order
            database.orderByChild('order').on('value', function (snapshot) {
                var links = snapshot.val();

                var linksContainer = document.getElementById('linksContainer');
                linksContainer.innerHTML = '';

                for (var linkId in links) {
                    var linkData = links[linkId];
                    var linkElement = document.createElement('div');
                    linkElement.innerHTML = `
                        <p>Title: ${linkData.title} (${linkId})</p>
                        <p>URL: <a href="${linkData.url}" target="_blank">${linkData.url}</a></p>
                        <p>Color: <span style="background-color: ${linkData.color}; padding: 4px 8px;">${linkData.color}</span></p>
                        <button onclick="editLink('${linkId}', '${linkData.title}', '${linkData.url}', '${linkData.color}')">Edit</button>
                        <button onclick="deleteLink('${linkId}')">Delete</button>
                        <hr>
                    `;

                    linksContainer.appendChild(linkElement);
                }
            });
        });
    });
}

function editLink(linkId, title, url, color, order) {
    isUserAuthorized(userInfo, function (isAuthorized) {
        document.getElementById('title').value = title;
        document.getElementById('url').value = url;

        // Check if the color is a named color and convert it to hexadecimal if needed
        color = convertToHexColor(color);
        if (color === null) {
            // Invalid color, use the original value
            color = document.getElementById('color').value;
        }
        document.getElementById('color').value = color;
        document.getElementById('order').value = order;
        document.getElementById('order').classList.add('not-allowed');
        document.getElementById('order').disabled = true;
        linkForm.dataset.linkId = linkId;
        document.getElementById('addLinkBtn').style.display = 'none';
        document.getElementById('updateLinkBtn').style.display = 'block';
    });
}



function convertToHexColor(color) {
    isUserAuthorized(userInfo, function (isAuthorized) {
        // Check if the color is a named color
        var namedColors = {
            maroon: '#800000',
            red: '#FF0000',
            // Add more named colors as needed
        };

        // Convert named color to hexadecimal if it exists in the namedColors object
        if (namedColors[color.toLowerCase()]) {
            return namedColors[color.toLowerCase()];
        }

        // Return the original color if it is not a named color
        return color;
    });
}

function onNodeSelectionChange() {
    var nodeSelection = document.getElementById('nodeSelection').value;
    const nodeRef = (nodeSelection === 'links2' || nodeSelection === 'podcast') ? nodeSelection : 'links';

    // beginMainProcess(nodeRef);

    // Retrieve and display existing links in the specified node
    firebase.database().ref(nodeRef).orderByChild('order').on('value', function (snapshot) {
        var links = snapshot.val();

        var linksContainer = document.getElementById('linksContainer');
        linksContainer.innerHTML = '';

        for (var linkId in links) {
            var linkData = links[linkId];
            var linkElement = document.createElement('div');
            linkElement.innerHTML = `
          <p>Title: ${linkData.title}</p>
          <p>URL: <a href="${linkData.url}" target="_blank">${linkData.url}</a></p>
          <p>Color: <span style="background-color: ${linkData.color}; padding: 4px 8px;">${linkData.color}</span></p>
          <button onclick="editLink('${linkId}', '${linkData.title}', '${linkData.url}', '${linkData.color}', '${linkData.order}')">Edit</button>
          <button onclick="deleteLink('${linkId}')">Delete</button>
          <hr>
        `;

            linksContainer.appendChild(linkElement);
        }
    });
}

function isUserAuthorized(userInfo, callback) {
    var userRef = firebase.database().ref('Users/' + userInfo.uid);
    userRef.child('auth').once('value', function (snapshot) {
        var authValue = snapshot.val();
        var isAuthorized = allowedDomain.includes(userInfo.domain) && (authValue === 1 || authValue === 2 || authValue === 3 || authValue === 4);
        callback(isAuthorized);
    });
}
