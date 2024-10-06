/*
Name: Aum Patel
Vers: 1.0
Date: Aug 16, 2024 (status of code is unchanged as of Version 1.0)
Description: This code contains the Mongoose schema for a No-SQL database. 
             It contains sensor data aswell as a timestamp adjusted for UTC-4.
*/

const mongoose = require('mongoose');
const moment = require('moment');

const Schema = mongoose.Schema;

const SensorsSchema = new Schema
(
    {
        tempC: Number,
        tds: Number,
        turbidity: Number,
        logTime: 
        {
            type: Date,
            default: moment().utc().subtract(4,"hours")
        }
    },     
    {
        versionKey: false,
        strict: false
    }
);
module.exports = mongoose.model('Sensors', SensorsSchema);
