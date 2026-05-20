#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\WebPageApp.h"
#pragma once
#include <Arduino.h>

static const char PAGE_APP_HTML[] PROGMEM = R"RCWEB(
<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <title>Управление</title>
  <link rel="stylesheet" href="/app.css">
</head>
<body>
  <div id="app"></div>
  <script src="/app.js"></script>
</body>
</html>

)RCWEB";
