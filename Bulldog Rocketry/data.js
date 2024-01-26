// Replace with your Firebase config
const firebaseConfig = {
    apiKey: "AIzaSyDc1bptlOncgHD3i-c0Br8lzMKYGnM_0Ys",
    authDomain: "bulldog-rocketry-a06cc.firebaseapp.com",
    databaseURL: "https://bulldog-rocketry-a06cc-default-rtdb.firebaseio.com",
    projectId: "bulldog-rocketry-a06cc",
    storageBucket: "bulldog-rocketry-a06cc.appspot.com",
    messagingSenderId: "313636010269",
    appId: "1:313636010269:web:d1d79f52fdd5b6e037c9f4",
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);

document.getElementById('downloadButton').addEventListener('click', async () => {
    try {
        const db = firebase.database();
        const ref = db.ref('STIStest'); // Change to your desired node

        // Fetch data from Firebase
        const snapshot = await ref.once('value');
        const data = snapshot.val();

        if (!data) {
            console.error('Data is null or undefined.');
            alert('Error fetching data. Check the console for details.');
            return;
        }

        // Convert data to CSV format
        const csvArray = Object.values(data);
        const csv = csvArray.join('\n');

        // Create a Blob containing the CSV data
        const blob = new Blob([csv], { type: 'text/csv' });

        // Create a link element to trigger the download
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);
        link.download = 'data.csv';

        // Append the link to the document and trigger the click event
        document.body.appendChild(link);
        link.click();

        // Remove the link from the document
        document.body.removeChild(link);
    } catch (error) {
        console.error(error);
        alert('Error fetching data. Check the console for details.');
    }
});
