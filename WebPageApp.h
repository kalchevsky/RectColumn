#pragma once
#include <Arduino.h>

static const char PAGE_APP_HTML[] PROGMEM = R"RCWEB(
<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <title>Управление</title>
  <link rel="stylesheet" href="/app.css?v=)RCWEB" FW_VERSION R"RCWEB(">
</head>
<body>
  <div id="app"></div>
  <script src="/app.js?v=)RCWEB" FW_VERSION R"RCWEB("></script>
</body>
</html>

)RCWEB";
