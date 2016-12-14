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

// Converts OpenWeather icon id to icon location within the Yahoo meteocons bitmap
var OWMToYahooIcon = function(owm_icon) {
  var yahoo_icon = 3200; //initialy not defined

  switch (owm_icon){
    case "01d": // clear sky
      yahoo_icon = 32; // sunny
      break;
    case "01n":
      yahoo_icon = 31; // moon
      break;
    case "02d": // few clouds
      yahoo_icon = 30; // partly cloudy day
      break;
    case "02n":
      yahoo_icon = 29; // partly cloudy night
      break;
    case "03d": // scattered clouds
      yahoo_icon = 26; // cloudy
      break;
    case "03n":
      yahoo_icon = 26; // cloudy
      break;
    case "04d": // broken clouds
      yahoo_icon = 26; // cloudy
      break;
    case "04n":
      yahoo_icon = 26; // cloudy
      break;
    case "09d": // shower rain
      yahoo_icon = 40; // two clouds rainy
      break;
    case "09n": 
      yahoo_icon = 40; // two clouds rainy
      break;
    case "10d": // rain
      yahoo_icon = 11; // one cloud rainy
      break;
    case "10n":
      yahoo_icon = 11; // one cloud rainy
      break;
    case "11d": // thunderstorm
      yahoo_icon = 4; // thunderstorm
      break;
    case "11n":
      yahoo_icon = 4; // thunderstorm
      break;
    case "13d": // snow
      yahoo_icon = 14; // snow
      break;
    case "13n":
      yahoo_icon = 14; // snow
      break;
    case "50d": // mist
      yahoo_icon = 20; // foggy
      break;
    case "50n":
      yahoo_icon = 20; // foggy
      break;
  }

  return yahoo_icon;

};

function locationSuccess(pos) {
  // Determine weather source from settings
  var settings = JSON.parse(localStorage.getItem('clay-settings'));
  var wsource = settings.WEATHER_SOURCE;
  var apikey;
  var url;
  
  if (wsource === "0") {
    // DarkSky //
    // Construct URL
    apikey = settings.WEATHER_APIKEY_DS;
    url = 'https://api.darksky.net/forecast/' + apikey + '/' + pos.coords.latitude + ',' + pos.coords.longitude;
    console.log('Weather request URL: ' + url);
    
    // Send request to DarkSky
    xhrRequest(url, 'GET', 
      function(responseText) {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);
  
        // Grab the data we want
        var temperature = json.currently.temperature;
        if (!settings.TEMP_F) {
          // Convert to Celcius
          temperature = (temperature - 32) / 1.8;
        }
        var icon = json.currently.icon;   
        
        
        
        // Assemble dictionary using our keys
        var dictionary = {
          'WEATHER_TEMP': Math.round(temperature),
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
  else if (wsource === "1") {
    // OpenWeatherMap //
    // Construct URL
    apikey = settings.WEATHER_APIKEY_OWM;
    url = 'http://api.openweathermap.org/data/2.5/weather?apikey=' + apikey + '&lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude;
    if (settings.TEMP_F) {
      url = url + '&units=imperial';
    }
    else {
      url = url + '&units=metric';
    }
    console.log('Weather request URL: ' + url);
    
    // Send request to DarkSky
    xhrRequest(url, 'GET', 
      function(responseText) {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);
  
        // Grab the data we want
        var temp = json.main.temp;
        var icon = json.weather[0].icon;
        
        console.log('temp: ' + temp);
        console.log('icon: ' + icon);
        
        
        // Assemble dictionary using our keys
        var dictionary = {
          'WEATHER_TEMP': Math.round(temp),
          'WEATHER_CODE': OWMToYahooIcon(icon),
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