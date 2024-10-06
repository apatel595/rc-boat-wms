/*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This program is for the backend application. Data is posted in the webserver using 
             CRUD operations and is also logged every 5 seconds in a MongoDB database.
             Please make sure to insert your own MongoDB connection string (bottom of code).

*/

const express = require('express')
const mongoose = require('mongoose')

const app = express()
const port = 8080

const Sensors = require('./model')

app.use(express.json());

let latestSensorData = null;

// HTTP GET Method
app.get('/live', (req, res) => {
   console.log("GET Request for live data");
   if (latestSensorData) {
       res.json(latestSensorData);
   } else {
       res.status(404).json({ error: 'No data available' });
   }
});

// HTTP POST Method 
app.post('/sensors', async (req, res) => {
   console.log('POST Request');
   let sensor = req.body;
   console.log('body:', sensor);

   try {
       sensor = new Sensors(sensor);
       sensor = await sensor.save();
       console.log('sensor:', sensor);
      
       latestSensorData = sensor;
       
       return res.status(201).json(sensor);
   } catch (error) {
       console.error('Error saving sensor data:', error);
       return res.status(500).json({ error: 'Internal Server Error' });
   }
});

// MongoDB Connection Success
mongoose.connection.on('connected', () => {
   console.log('MongoDb connected');
});

// MongoDB Connection Fail
mongoose.connection.on('error', (err) => {
   console.log('Error connecting MongoDb', err);
});

app.listen(port, async() => {
   await mongoose.connect('ENTER YOUR MONGODB CONNECTION STRING HERE'); // replace with MongoDB connection string
   console.log(`Example app listening on port ${port}`)
   
})

