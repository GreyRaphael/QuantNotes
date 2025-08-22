# ETF

- [ETF](#etf)
  - [fields](#fields)
  - [service](#service)


## fields

discount_rate= 100 * ( close - netvalue ) / netvalue


turnover = volume / total_share

```py
def merge_etf_bar1d(year: int):
    """join etf bar1d between aiquant & wind"""
    df_wind = pl.read_ipc(f"wind_bar1d_etf/{year}.ipc", memory_map=False)
    df_ai = pl.read_ipc(f"aiquant-ETF-bar1d/{year}.ipc", memory_map=False)
    df_final = df_ai.join(df_wind.select("code", "dt", "turnover", (pl.col("netvalue") * 1e4).round(0).cast(pl.UInt32)), on=["code", "dt"], how="inner")
    df_final.sort(["code", "dt"]).write_ipc(f"wind_aiquant_etf_bar1d/{year}.ipc", compression="zstd")
    print("finish", year)
```

## service

1. add `aietf.service` file
2. start service
3. check port occupy: `sudo netstat -tulnp | grep :8000`

```bash
# /etc/systemd/system/aietf.service
[Unit]
Description=My FastAPI (Uvicorn) Service
After=network.target

[Service]
Type=simple
# run as your user
User=gewei
Group=gewei
WorkingDirectory=/home/gewei/aietf

# ensure systemd sees your ~/.local/bin on PATH
Environment=PATH=/home/gewei/.local/bin:/usr/local/bin:/usr/bin:/bin

# use the real binary path (here uvicorn)
ExecStart=/home/gewei/.local/bin/uv run fastapi dev --host=0.0.0.0 --port=8080
# or if you invoke uvicorn directly:
# ExecStart=/opt/myproject/venv/bin/uvicorn app.main:app --host 0.0.0.0 --port 8000 --workers 4

Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

```bash
# reload new service
sudo systemctl daemon-reload
sudo systemctl enable aietf.service
sudo systemctl start aietf.service
sudo systemctl restart aietf.service
sudo systemctl status aietf.service
# check log
sudo journalctl -u aietf.service -f
```

```bash
# remove aietf.service completely
sudo systemctl stop aietf.service
sudo systemctl disable aietf.service

# Find the location of the service file
systemctl cat aietf.service

# Then, delete the file (replace /path/to/aietf.service with the actual path)
sudo rm /path/to/aietf.service

# Reload the systemd daemon: This ensures that systemd recognizes the changes you've made to the service configuration.
sudo systemctl daemon-reload
```