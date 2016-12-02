// Clay (app config)
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

// Converts DarkSky summary to icon location within the Yahoo meteocons bitmap
var DarkSkyIconToYahooIcon = function(darksky_icon) {
  var yahoo_icon = 3200; //initialy not defined

  switch (darksky_icon){
    case "clear-day":
      yahoo_icon = 32; // sunny
      break;
    case "clear-night":
      yahoo_icon = 31; // clear night
      break;
    case "rain":
      yahoo_icon = 11; // showers
      break;
    case "snow":
      yahoo_icon = 16; // snow
      break;
    case "sleet": 
      yahoo_icon = 18; // sleet
      break;
    case "wind": 
      yahoo_icon = 24; // windy
      break;
    case "fog": 
      yahoo_icon = 20; // foggy
      break;
    case "cloudy":
      yahoo_icon = 26; // cloudy
      break;
    case "partly-cloudy-day":
      yahoo_icon = 30; // partly cloudy day
      break;
    case "partly-cloudy-night":
      yahoo_icon = 29; // partly cloudy night
      break;
  }

  return yahoo_icon;

};

function locationSuccess(pos) {
  // Get API Key from clay storage
  var settings = JSON.parse(localStorage.getItem('clay-settings'));
  var apikey = settings.APIKey_DarkSky;
  
  // Construct URL
  var url = 'https://api.darksky.net/forecast/' + apikey + '/' + pos.coords.latitude + ',' + pos.coords.longitude;
  console.log('Weather request URL: ' + url);
  
  // Send request to DarkSky
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Grab the data we want
      var temperature = Math.round(json.currently.temperature);
      if (!settings.TEMP_F) {
        // Convert to Celcius
        temperature = (temperature - 32) / 1.8;
      }
      var icon = json.currently.icon;   
      
      
      
      // Assemble dictionary using our keys
      var dictionary = {
        'WEATHER_TEMP': temperature,
        'WEATHER_CODE': DarkSkyIconToYahooIcon(icon),
      };
    
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  // Attempt to get GPS coords
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    Pebble.sendAppMessage({'JSReady': true });
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }
);