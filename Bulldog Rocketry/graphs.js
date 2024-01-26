document.addEventListener('DOMContentLoaded', async () => {
    try {
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
        // firebase.initializeApp(firebaseConfig);

        const db = firebase.database();
        const ref = db.ref('STIStest'); // Change to your desired node

        // Fetch data from Firebase
        const snapshot = await ref.once('value');
        const dataArray = Object.values(snapshot.val() || []);

        // Filter out lines that do not contain the specified format
        const validLinesArray = dataArray
            .filter(line => line.includes(','))
            .filter(line => !line.startsWith('system state:'));

        // Split lines into columns
        const columnsArray = validLinesArray.map(line => line.split(','));

        // Extract time and TC columns
        const timeAxis = columnsArray.map(columns => parseInt(columns[0]));
        const TC1 = columnsArray.map(columns => parseFloat(columns[1]));
        const TC2 = columnsArray.map(columns => parseFloat(columns[2]));
        const LC = columnsArray.map(columns => parseFloat(columns[3]));
        const P1 = columnsArray.map(columns => parseFloat(columns[4]));
        const P2 = columnsArray.map(columns => parseFloat(columns[5]));

        // Create a canvas element for TC1 graph
        createGraph('Temp 1', timeAxis, TC1);

        // Create a canvas element for TC2 graph
        createGraph('Temp 2', timeAxis, TC2);

        // Create a canvas element for LC graph
        createGraph('Thrust', timeAxis, LC);

        // Create a canvas element for P1 graph
        createGraph('Pressure 1', timeAxis, P1);

        // Create a canvas element for P2 graph
        createGraph('Pressure 2', timeAxis, P2);
    } catch (error) {
        console.error(error);
        alert('Error fetching data. Check the console for details.');
    }
});

function createGraph(tcLabel, timeAxis, tcData) {
    // Create a container div for the graph
    const graphContainer = document.createElement('div');
    graphContainer.style.marginBottom = '20px';
    document.body.appendChild(graphContainer);

    // Create a canvas element for the graph
    const canvas = document.createElement('canvas');
    canvas.width = 600;
    canvas.height = 400;
    graphContainer.appendChild(canvas);

    const ctx = canvas.getContext('2d');
    if (ctx) {
        console.log(`Canvas context found for ${tcLabel}.`);

        // Define chart dimensions
        const chartWidth = 600;
        const chartHeight = 400;

        // Find the min and max values excluding NaN
        const minValue = Math.min(...tcData.filter(val => !isNaN(val)));
        const maxValue = Math.max(...tcData.filter(val => !isNaN(val)));

        // Scale factors for the chart
        const scaleX = chartWidth / (timeAxis.length - 1);
        const scaleY = chartHeight / (maxValue - minValue || 1);

        console.log(`ScaleX for ${tcLabel}:`, scaleX);
        console.log(`ScaleY for ${tcLabel}:`, scaleY);

        // Draw the line chart
        ctx.beginPath();
        ctx.strokeStyle = `rgb(${Math.random() * 255}, ${Math.random() * 255}, ${Math.random() * 255})`;

        ctx.moveTo(0, chartHeight - (tcData[0] - minValue) * scaleY);

        for (let i = 1; i < timeAxis.length; i++) {
            const x = i * scaleX;
            const y = chartHeight - (tcData[i] - minValue) * scaleY;
            ctx.lineTo(x, y);
        }

        ctx.stroke();

        // Add labels and styling
        ctx.font = '14px Arial';
        ctx.fillStyle = 'black';
        ctx.fillText(`${tcLabel}`, 10, 20);
        ctx.fillText('Time', chartWidth / 2, chartHeight - 10);
        // ctx.fillText(tcLabel, 10, chartHeight / 2);

        // Draw axes
        ctx.strokeStyle = 'black';
        ctx.lineWidth = 2;
        ctx.moveTo(0, 0);
        ctx.lineTo(0, chartHeight);
        ctx.lineTo(chartWidth, chartHeight);
        ctx.stroke();
    } else {
        console.error(`Canvas context not found for ${tcLabel}.`);
    }
}
