/*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This program fetches sensor data for frontend application
*/

document.addEventListener('DOMContentLoaded', () => {
    const temperatureEl = document.getElementById('temperature');
    const tdsEl = document.getElementById('tds');
    const turbidityEl = document.getElementById('turbidity');

    function fetchSensorData() 
    {
        fetch('/live')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(data => {
                temperatureEl.textContent = `${data.tempC} °C`;
                tdsEl.textContent = data.tds;
                turbidityEl.textContent = data.turbidity;
            })
            .catch(error => {
                console.error('Fetch operation error:', error);
            });
    }

    // Fetch data every 5 seconds
    fetchSensorData(); 
    setInterval(fetchSensorData, 5000); 
});
