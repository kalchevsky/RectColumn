RectColumn test commands:

Host-side:
`python -m unittest -v tools.test_logic_scheme`
`python -m unittest -v tools.test_logic_full_matrix`

Live EMU API:
`set RECTCOLUMN_BASE_URL=http://192.168.4.1`
`python -m unittest -v tools.test_api_emu_channel_sensor_matrix`

Linux/macOS:
`RECTCOLUMN_BASE_URL=http://192.168.4.1 python -m unittest -v tools.test_api_emu_channel_sensor_matrix`
