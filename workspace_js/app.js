var MicroGear = require('microgear');
const readline = require('readline');
var glob = require("glob")
var path = require('path');
var fs = require('fs');
//var sleep = require('sleep');

const KEY    = "xxxxxxxxxxx";
const SECRET = "xxxxxxxxxxx";
const APPID     = "xxxxxxxx";

var online_gear = {};

var microgear = MicroGear.create({
    key : KEY,
    secret : SECRET
});

microgear.on('connected', function() {
    console.log('Connected...');
    microgear.setAlias("uploader");
    microgear.subscribe("/pong");
    
    online_gear = [];
    microgear.publish("/ping","abc123");
    setTimeout(function(){ rl.prompt(); }, 3000);    
});

microgear.on('message', function(topic,body) {
    console.log('incoming : '+topic+' : '+body);
    if(topic == "/"+APPID+"/pong"){
      online_gear.push(body);
    }
});

microgear.connect(APPID);
//------- console ---------//
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  prompt: 'SYS> '
});

rl.on('line', (line) => {
  switch(line.trim()) {
    case 'upload':      
      glob("**/*.bin", {'realpath':true}, function (er, files) {
        if(files.length > 0){
          console.log('Uploading ... File : '+files[0] + " To : "+online_gear[0]);          
          upload3(files[0],online_gear[0]);
        }else{
          console.log("Bin file not found!");
        }
      });
      break;
    case 'ping':
      console.log("Pinging");
      microgear.publish("/ping");
      break;
    case 'list':
      console.log("list all online devices");
      console.log(online_gear);
      break;
    default:
      console.log("Try again!");
      break;
  }
  rl.prompt();
}).on('close', () => {
  console.log('Bye!');
  process.exit(0);
});

var CHUNK_SIZE = 512; // 10KB
var buffer = new Buffer(CHUNK_SIZE);


function upload3(file,gear_name){
  fs.open(file, 'r', function(err, fd) {
    fs.fstat(fd, function(err, stats) {
        var fileSize=stats.size,
            bytesRead = 0,
            last = false;        
        
        console.log("Uploading " + file + " Data : " + fileSize+" Bytes");
        microgear.publish("/"+gear_name+"_upload",fileSize+"");

        //while (bytesRead < fileSize) {            
        function readnext() {          
          fs.read(fd, buffer, 0, CHUNK_SIZE, null);            
          var data;
          if ((bytesRead + CHUNK_SIZE) > fileSize) {
            data = buffer.slice(0, fileSize - bytesRead);
            fs.close(fd);
            last =true;
          }else{
            data = buffer;
          }
          bytesRead += CHUNK_SIZE;
          console.log("Sending : " +bytesRead + ":" + fileSize + " ==> " + data.length);
          console.log(data);
          microgear.publish("/"+gear_name+"_upload",data.toString('ascii')); //<<<<<
          if(!last){
            setTimeout(function(){ console.log("tick"); readnext(); },500);
          }          
        }
        setTimeout(function(){readnext();},3000);
        console.log("Done Upload!");
    });
  });
}

function getFilesizeInBytes(filename) {
  var stats = fs.statSync(filename)
  var fileSizeInBytes = stats["size"]
  return fileSizeInBytes
}
