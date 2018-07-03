// 	Author        : Kevin Too
// 	Latest version: 19 Jun 2018
//
// 	After each change in this script, you must publish as web app and update the project version. 
// 	Otherwise the changes won't take effect. 
//	
//  This script is linked to the following Google spreadsheet:
// 	https://docs.google.com/spreadsheets/d/1J-X0FEM01D2KrmVig0ToSqcYUvMaXh25niFc7-94vAI/edit?usp=sharing
//	
//	The script can be tested with the following URL:
// 	https://script.google.com/macros/s/AKfycbwxqesb-dBfgPk8s9MQ_Hj7-Dc0fkCajv2XWiDOgY8bN8EY_AM/exec?mode=1&data={"A0":"20","A1":"21","A2":"22","A3":"23","A4":"24","A5":"25","A6":"26","A7":"27","A8":"22","A9":"23","A10":"24","A11":"25","A12":"26","A13":"27","A14":"28","A15":"29","A16":"24","A17":"25","A18":"26","A19":"27","A20":"28","A21":"29","A22":"30","A23":"31","A24":"26","A25":"27","A26":"28","A27":"29","A28":"30","A29":"31","A30":"32","A31":"33","A32":"28","A33":"29","A34":"30","A35":"31","A36":"32","A37":"33","A38":"34","A39":"35","A40":"30","A41":"31","A42":"32","A43":"33","A44":"34","A45":"35","A46":"36","A47":"37","A48":"32","A49":"33","A50":"34","A51":"35","A52":"36","A53":"37","A54":"38","A55":"39","A56":"34","A57":"35","A58":"36","A59":"37","A60":"38","A61":"39","A62":"40","A63":"41","A64":"25","A65":"101234","B0":"62","B1":"67","B2":"68","B3":"69","B4":"70","B5":"71","B6":"72","B7":"50","B8":"0"}

function doGet(e){	// handle GET requests
	var dateTime = new Date();
	var flag = 0;
	var outputString;
	var hottestPixelLimit = 40;
	Logger.log("--- doGet ---");
  
//  if (e!=null){
//    var mode = e.parameter.mode;                      // if 1: data contains 64 pixels of GridEYE. Else, data contains hottest pixel of each row & its index.
//    var dataReceived =  JSON.parse(e.parameter.data); // e.parameter.data is a string. You can only parse a string, not a data object. Parsing a data object will throw an "Unexpected token: o" error.
//  }

  try {
    if (e == null){ // initialize dummy parameters if testing the script before actual deployment (for running from within GScript page)
      flag = 1;
      e={};
      e.parameter = {
        "data":{
        "A0": "0",
        "A1": "1",
        "A2": "2",
        "A3": "3",
        "A4": "4",
        "A5": "5",
        "A6": "6",
        "A7": "7",
        "A8": "8",
        "A9": "9",
        "A10": "10",
        "A11": "11",
        "A12": "12",
        "A13": "13",
        "A14": "14",
        "A15": "15",
        "A16": "16",
        "A17": "17",
        "A18": "18",
        "A19": "19",
        "A20": "20",
        "A21": "21",
        "A22": "22",
        "A23": "23",
        "A24": "24",
        "A25": "25",
        "A26": "26",
        "A27": "27",
        "A28": "28",
        "A29": "29",
        "A30": "30",
        "A31": "31",
        "A32": "32",
        "A33": "33",
        "A34": "34",
        "A35": "35",
        "A36": "36",
        "A37": "37",
        "A38": "38",
        "A39": "39",
        "A40": "40",
        "A41": "41",
        "A42": "42",
        "A43": "43",
        "A44": "44",
        "A45": "45",
        "A46": "46",
        "A47": "47",
        "A48": "48",
        "A49": "49",
        "A50": "50",
        "A51": "51",
        "A52": "52",
        "A53": "53",
        "A54": "54",
        "A55": "55",
        "A56": "56",
        "A57": "57",
        "A58": "58",
        "A59": "59",
        "A60": "60",
        "A61": "61",
        "A62": "62",
        "A63": "63",
        "A64": "64",
        "A65": "65",
        "B0": "66",
        "B1": "67",
        "B2": "68",
        "B3": "69",
        "B4": "70",
        "B5": "71",
        "B6": "72",
        "B7": "73",
        "B8": "74"},
        "mode":"1"
      };
      var dataReceived = e.parameter.data;
    } else {
      var dataReceived =  JSON.parse(e.parameter.data); 
    }
    
    var mode = e.parameter.mode;                      
    save_data1(mode, dataReceived); // argument should be a JSON object, not a string.

//    Logger.log(JSON.stringify(e.parameter));
    
    if(flag == 0){
      outputString = "Data received OK\nMode: "+mode.toString();
    } else {
      outputString = "Dummy Test\n";
    }
    
    var hottestPixel=0;
    for (var data in dataReceived){
      if(data == "B0"){break;}
      else{
        if (Number(dataReceived[data])>hottestPixel){
          hottestPixel = Number(dataReceived[data]);
        }
      }
    } //end for
    
    if (hottestPixel>hottestPixelLimit){ 				// send Gmail notifications if hottest pixel exceeds specified treshold
      var mailTitle = "FireFly Script notification";
      var mailMessage = "Timestamp: " + dateTime.toString() + "\n";
      mailMessage += "Hottest Pixel: "+hottestPixel+" degrees Celsius\n";
      mailMessage += "Notification treshold: "+hottestPixelLimit+" degrees Celsius\n";
      GmailApp.sendEmail("kit088@gmail.com", mailTitle, mailMessage);
      Logger.log("Hottest Pixel: "+hottestPixel+"\nEmail sent!");
    }

    outputString += dateTime.toString();
    //Logger.log(JSON.stringify(outputString));
    Logger.log("— doGet end—");
    return ContentService.createTextOutput(outputString); // stringify the JSON object and print on screen. 
  } catch(error) {
      Logger.log(error);
      return ContentService.createTextOutput("oops...." + error.message
                                              + "\nDate: " + new Date());
  }
}

//---------------------------------------------------------------------------
function save_data1(mode, dataSet){
  Logger.log("— save_data1 —");
  Logger.log(LanguageApp.translate('This is just some dummy text to test Google\'s LanguageApp.translate method.', 'en', 'fr'));
  try {
    var dateTime = new Date();

    // Paste the URL of the spreadsheeet starting from https thru /edit
    // For e.g.: https://docs.google.com/…./edit
    var ss = SpreadsheetApp.openByUrl("https://docs.google.com/spreadsheets/d/1J-X0FEM01D2KrmVig0ToSqcYUvMaXh25niFc7-94vAI/edit");
    var summarySheet = ss.getSheetByName("Summary");
    var dataLoggerSheet0 = ss.getSheetByName("DataLogger0");
    var dataLoggerSheet1 = ss.getSheetByName("DataLogger1");
    var dataLoggerSheet2 = ss.getSheetByName("DataLogger2");

    // Clear grid
    var pixelGrid = summarySheet.getRange(3, 2, 10, 9);
    pixelGrid.clearContent();

    // Get last edited row from DataLogger sheet
    var row0 = dataLoggerSheet0.getLastRow() + 1;
    var row1 = dataLoggerSheet1.getLastRow() + 1;
    var row2 = dataLoggerSheet2.getLastRow() + 2;

    // Start Populating the data
    const alphabetCount = 26;
    var i = 0;
    var colOffset = 1; // To account for dateTime column
    var col="";
    var objSize = Object.keys(dataSet).length;
    
    if(mode==0){
      dataLoggerSheet0.getRange("A" + row0).setValue(dateTime);                  // dateTime goes in column A
    } else if (mode == 1) {
      dataLoggerSheet1.getRange("A" + row1).setValue(dateTime);
    } else {
      dataLoggerSheet2.getRange("A" + row2).setValue(dateTime);
    }
    
    if (mode < 2){
      for(var data in dataSet){
        var multiplier = ((i+colOffset)-((i+colOffset)%alphabetCount))/alphabetCount;
        if(multiplier==0){
          col = String.fromCharCode(65+colOffset+i); // start logging data in column B onwards. ASCII 65 is char 'A'.
        }
        else{col = String.fromCharCode(65+multiplier-1)+String.fromCharCode(65+i-(multiplier*alphabetCount)+colOffset);} // for columns AA, AB, AC...up to ZZ
  
        if(mode==0){
          dataLoggerSheet0.getRange(col + row0).setValue(dataSet[data]); // value
        } else {
          dataLoggerSheet1.getRange(col + row1).setValue(dataSet[data]); // value
        }
        i = i+1;
      } //end for
      
      // Update summary sheet
      var rangeToCopy0;
      for(var j=0; j<8; j++){
        rangeToCopy0 = dataLoggerSheet1.getRange(row1, (2+(8*j)), 1, 8);     // Latest GridEYE data (Source)
        rangeToCopy0.copyTo(summarySheet.getRange((3+j), 2), {contentsOnly:true});  //                     (Destination)
      }
      var rangeToCopy1 = dataLoggerSheet1.getRange(row1, 66, 1, 11);           // Latest data from other sensors (Source)
      rangeToCopy1.copyTo(summarySheet.getRange(3, 10), {contentsOnly:true}); //                                (Destination)
      
    } else {
      for(var data in dataSet){
        if(i<64){
          col = String.fromCharCode(65+colOffset+(i%8));
        }  else {
          col = String.fromCharCode(65+9+(i-64));
        }
        if (i>0 && i<64 && i%8 == 0){row2 += 1;};
        dataLoggerSheet2.getRange(col + row2).setValue(dataSet[data]);        
        i = i+1;
      };
      var range = dataLoggerSheet2.getRange('A'+row2+':T'+row2);  // Draw cell borders
      range.setBorder(null, null, true, null, null, null);
      range = dataLoggerSheet2.getRange('B'+(row2-8)+':I'+row2);
      range.setBorder(null, true, null, true, null, null);
      range = dataLoggerSheet2.getRange('M'+(row2-8)+':R'+row2);
      range.setBorder(null, true, null, true, null, null);
      range = dataLoggerSheet2.getRange('T'+(row2-8)+':T'+row2);
      range.setBorder(null, true, null, true, null, null);
      
      // Update summary sheet
      var rangeToCopy2 = dataLoggerSheet2.getRange((row2-7), 2, 8, 8);     // Latest GridEYE data (Source)
      rangeToCopy2.copyTo(summarySheet.getRange(3, 2), {contentsOnly:true});  //                     (Destination)
      rangeToCopy2 = dataLoggerSheet2.getRange(row2, 10, 1, 11);           // Latest data from other sensors (Source)
      rangeToCopy2.copyTo(summarySheet.getRange(3, 10), {contentsOnly:true}); //                                (Destination)
    }
    summarySheet.getRange("A3").setValue(dateTime);                        // Last modified date



//      // Start Populating the data
//      for(var data in dataSet){ // actually this is writing in columns A and B with the same data for each loop! Can move this out of the loop. 
//        dataLoggerSheet.getRange("A" + row).setValue(row -1); // ID   - not necessary
//        dataLoggerSheet.getRange("B" + row).setValue(dateTime); // dateTime
//  //      dataLoggerSheet.getRange("C" + row).setValue(data); // This works for uploading the name of tag, but is not necessary. Just need values. 
//        dataLoggerSheet.getRange("D" + row).setValue(dataSet[data]); // value
//        dataLoggerSheet.getRange("D" + row).setValue(dataSet[data]); // value
//  // where is the new row inserted? 
//  // It is not. You edit the existing row, and increment the row index each time. 
//        row += 1;
//      };

  } //end try

  catch(error) {
    Logger.log(JSON.stringify(error));
  }

  Logger.log("— save_data end—");
}
