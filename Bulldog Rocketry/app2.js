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
firebase.initializeApp(firebaseConfig);

// Reference to your Firebase database
var database = firebase.database().ref('links2');

// Retrieve data from Firebase and populate the links
database.once('value').then(function (snapshot) {
    var links = snapshot.val();

    var linksContainer = document.getElementById('links-container');
    linksContainer.innerHTML = '';

    for (var link in links) {
        var linkData = links[link];
        var linkContainer = document.createElement('div'); // Create a <div> container for the link
        var linkElement = document.createElement('a');
        linkElement.href = linkData.url;
        linkElement.textContent = linkData.title;
        linkElement.className = 'link';
        linkElement.style.backgroundColor = linkData.color;
        var brightness = calculateBrightness(linkData.color);
        linkElement.style.color = brightness > 130 ? '#000' : '#FFF';
        linkElement.target = '_blank';
        linkContainer.appendChild(linkElement); // Append the link to the <div> container
        linksContainer.appendChild(linkContainer); // Append the <div> container to the links container
        // linksContainer.appendChild(document.createElement('br')); // Insert a line break after each link container
    }
    
    
    function calculateBrightness(color) {
        var r, g, b, brightness;
        color = +('0x' + color.slice(1).replace(
            color.length < 5 && /./g, '$&$&'
        ));
        r = color >> 16;
        g = color >> 8 & 255;
        b = color & 255;
        brightness = (r * 299 + g * 587 + b * 114) / 1000;
        return brightness;
    }
});

// Get the overlay element
var overlay = document.getElementById("overlay");

// Function to remove the overlay after a delay
function removeOverlay() {
  overlay.classList.add("fade-out");
  setTimeout(function() {
    overlay.style.display = "none";
  }, 2000); // Delay in milliseconds
}

// Call the function to remove the overlay after 2 seconds
setTimeout(removeOverlay, 500);
