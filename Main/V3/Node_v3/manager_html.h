const char manager_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>%DEVICE_NAME%</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #f7f7f7;
    }

    #submit {
      width: 120px;
    }

    #edit_path {
      width: 250px;
    }

    #delete_path {
      width: 250px;
    }

    #spacer_50 {
      height: 50px;
    }

    #spacer_20 {
      height: 20px;
    }

    table {
      background-color: #dddddd;
      border-collapse: collapse;
      width: 650px;
    }

    td,
    th {
      border: 1px solid #dddddd;
      text-align: left;
      padding: 8px;
    }

    #first_td_th {
      width: 400px;
    }

    tr:nth-child(even) {
      background-color: #ffffff;
    }

    #format_notice {
      color: #ff0000;
    }

    #left_div {
      float: left;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #right_div {
      float: right;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #right_div {
      float: right;
      box-sizing: border-box;
      vertical-align: middle;
      display: inline-block;
    }

    #wrap_div {
      margin: auto;
      text-align: center;
    }
  </style>
  <script>
    function validateFormUpdate() {
      var inputElement = document.getElementById('update');
      var files = inputElement.files;
      if (files.length == 0) {
        alert("File Not Selected");
        return false;
      }
      var value = inputElement.value;
      var dotIndex = value.lastIndexOf(".") + 1;
      var valueExtension = value.substring(dotIndex);
      if (valueExtension != "bin") {
        alert("Not A Firmware File");
        return false;
      }
    }
    function confirmFormat() {
      var text = "まじで...?www";
      if (confirm(text) == true) {
        return true;
      }
      else {
        return false;
      }
    }
  </script>
</head>

<body>
  <center>
    <h2>%DEVICE_NAME%</h2>
    <div id="spacer_20"></div>



    <table>
      <td align="center" valign="top">
        <center>
          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Device INFO</legend>
            <table>
              <th scope="col">WiFi SSID</th>
              <td>%SSID%</td>
              </tr>
              <tr>
                <th scope="col">RSSI</th>
                <td>%RSSI% (%WIFI_QUALITY%)</td>
              </tr>
              <tr>
                <th scope="col">Device IP</th>
                <td>%IP%</td>
              </tr>
              <tr>
                <th scope="col">MAC</th>
                <td>%MAC%</td>
              </tr>
              <tr>
                <th scope="col">CH1</th>
                <td>%CH1_DeviceNo%</td>
                <th>Enable</th>
                <td>%CH1_Live%</td>
              <tr>
                <th scope="col">C_W, Flow, C_D</th>
                <td>%CH1_Curr_W%</td>
                <td>%CH1_Flow_W%</td>
                <td>%CH1_Curr_D%</td>
              </tr>
              <tr>
                <th scope="col">EndDelay_W, D</th>
                <td>%CH1_EndDelay_W%</td>
                <td>%CH1_EndDelay_D%</td>
                <td></td>
              </tr>
              <tr>
                <th scope="col">CH2</th>
                <td>%CH2_DeviceNo%</td>
                <th>Enable</th>
                <td>%CH2_Live%</td>
              <tr>
                <th scope="col">C_W, Flow, C_D</th>
                <td>%CH2_Curr_W%</td>
                <td>%CH2_Flow_W%</td>
                <td>%CH2_Curr_D%</td>
              </tr>
              <tr>
                <th scope="col">EndDelay_W, D</th>
                <td>%CH2_EndDelay_W%</td>
                <td>%CH2_EndDelay_D%</td>
                <td></td>
              </tr>
              <tr>
                <th scope="col">Flash Size</th>
                <td>%FlashSize% KiB</td>
              </tr>
              <tr>
                <th scope="col">Heap Memory</th>
                <td>%Heap% KiB Left</td>
              </tr>
              <tr>
                <th scope="col">F/W Build Date</th>
                <td>%BUILD_VER%</td>
              </tr>
            </table>
          </fieldset>
        </center>
      </td>
      <td align="center" valign="top">
        <center>
          <table>
            <tr>
              <td>
                <center>
                  <fieldset style="width:325px;background-color: #f7f7f7;">
                    <legend>WiFi Setting</legend>
                    <form method="POST" action="/wifi">
                      <p>
                        <input type="text" id="WiFi_SSID" name="WiFi_SSID" placeholder="SSID"><br>
                        <input type="text" id="WiFi_PASS" name="WiFi_PASS" placeholder="Password"><br>
                        <!-- <label for="ip">IP Address</label>
                    <input type="text" id="WiFi_IP" name="WiFi_IP" value="192.168.1.200"><br>
                    <label for="gateway">Gateway Address</label>
                    <input type="text" id="WiFi_Gateway" name="WiFi_Gateway" value="192.168.1.1"><br> -->
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
              <td>
                <center>
                  <fieldset style="width:325px;background-color: #f7f7f7;">
                    <legend>HTTP AUTH Setting</legend>
                    <form method="POST" action="/auth">
                      <p>
                        <input type="text" id="AUTH_ID" name="AUTH_ID" placeholder="ID"><br>
                        <input type="text" id="AUTH_PASSWD" name="AUTH_PASSWD" placeholder="Password"><br>
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
            </tr>
            <tr></tr>
            <tr>
              <td>
                <center>
                  <fieldset style="width:325px;height:100px;background-color: #f7f7f7;">
                    <legend>CH1_Setting</legend>
                    <form method="POST" action="/CH1">
                      <p>
                        <select name="CH1">
                          <option value="none" selected>Select Command</option>
                          <option value="DeviceNo">DeviceNo</option>
                          <option value="Current_Wash">Current_Wash</option>
                          <option value="Flow_Wash">Flow_Wash</option>
                          <option value="Current_Dry">Current_Dry</option>
                          <option value="EndDelay_Wash">EndDelay_Wash</option>
                          <option value="EndDelay_Dry">EndDelay_Dry</option>
                          <option value="Enable">Enable</option>
                        </select>
                        <input type="text" id="Command" name="value" placeholder="value"><br>
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
              <td>
                <center>
                  <fieldset style="width:325px;height:100px;background-color: #f7f7f7;">
                    <legend>CH2_Setting</legend>
                    <form method="POST" action="/CH2">
                      <p>
                        <select name="CH2">
                          <option value="none" selected>Select Command</option>
                          <option value="DeviceNo">DeviceNo</option>
                          <option value="Current_Wash">Current_Wash</option>
                          <option value="Flow_Wash">Flow_Wash</option>
                          <option value="Current_Dry">Current_Dry</option>
                          <option value="EndDelay_Wash">EndDelay_Wash</option>
                          <option value="EndDelay_Dry">EndDelay_Dry</option>
                          <option value="Enable">Enable</option>
                        </select>
                        <input type="text" id="Command" name="value" placeholder="value"><br>
                      <div id="spacer_10"></div>
                      </p>
                      <input type="submit" value="Submit">
                    </form>
                  </fieldset>
                </center>
              </td>
            </tr>
          </table>

          <div id="spacer_20"></div>

          

          <div id="spacer_20"></div>

          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Firmware Update</legend>
            <div id="spacer_20"></div>
            <form method="POST" action="/update" enctype="multipart/form-data">
              <table>
                <tr>
                  <td id="first_td_th">
                    <input type="file" id="update" name="update">
                  </td>
                  <td>
                    <input type="submit" id="submit" value="Start" onclick="return validateFormUpdate()">
                  </td>
                </tr>
              </table>
            </form>
            <div id="spacer_20"></div>
          </fieldset>

          <div id="spacer_20"></div>

          <fieldset style="width: 700px;background-color: #f7f7f7;">
            <legend>Device Reboot</legend>
            <form method="GET" action="/reboot" enctype="multipart/form-data">
              <input type="submit" id="submit" value="REBOOT" onclick="return confirmFormat()">
            </form>
          </fieldset>
        </center>
      </td>

    </table>

  </center>
  <iframe style="display:none" name="self_page"></iframe>
</body>

</html> )rawliteral";
