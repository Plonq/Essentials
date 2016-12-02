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
        "type": "input",
        "messageKey": "APIKey_DarkSky",
        "label": "DarkSky (forecast.io) API Key"
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