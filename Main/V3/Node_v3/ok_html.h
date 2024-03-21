const char ok_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Firmware Update Success</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>Firmware Update Success</h2>
      <div id="spacer_50"></div>
      <button onclick="window.location.href='/';">Return</button>
    </center>
  </body>
</html>
)rawliteral";
const char failed_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update Fail</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>Firmware Update Fail</h2>
      <div id="spacer_50"></div>
      <button onclick="window.location.href='/';">Return</button>
    </center>
  </body>
</html>
)rawliteral";
