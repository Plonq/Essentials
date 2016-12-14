module.exports = [
  {
    "type": "heading",
    "defaultValue": "Essentials Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Configure the Essentials watchface here"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Time/Date"
      },
      {
        "type": "select",
        "messageKey": "DATE_FORMAT",
        "label": "Date Format",
        "options": [
          { 
            "label": "MON 01 JAN",
            "value": "0"
          },
          { 
            "label": "MON JAN 01",
            "value": "1"
          }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "select",
        "messageKey": "WEATHER_SOURCE",
        "label": "Weather Source",
        "options": [
          { 
            "label": "DarkSky (forecast.io)",
            "value": "0"
          },
          { 
            "label": "OpenWeatherMap",
            "value": "1"
          }
        ]
      },
      {
        "type": "input",
        "messageKey": "WEATHER_APIKEY_DS",
        "label": "DarkSky API Key"
      },
      {
        "type": "input",
        "messageKey": "WEATHER_APIKEY_OWM",
        "label": "OpenWeatherMap API Key"
      },
      {
        "type": "toggle",
        "messageKey": "TEMP_F",
        "label": "Use Fahrenheit (F)",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];