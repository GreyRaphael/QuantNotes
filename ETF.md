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

Best Method: 用反向 SSH 隧道（reverse SSH tunnel）把 Windows 本地的 localhost:8000 暴露到 remote CentOS，然后第 3 台机器访问 CentOS 的转发端口即可。
- Windows 很可能在家/公司内网，CentOS 是公网云主机 → Windows 发起到 CentOS 的连接更容易通过 NAT/防火墙。
- 反向 SSH 隧道简单、稳定、可用密钥认证并且加密传输。

```bash
# in centos
# step1: modify sshd_config
sudo vi /etc/ssh/sshd_config
# allow GatewayPorts
GatewayPorts yes

sudo systemctl restart sshd

# step2: centos防火墙打开8080端口
```

```bash
# in win10, 将id_ed25519.pub放到centos ~/.ssh/authorized_keys
ssh -o ServerAliveInterval=60 -o ServerAliveCountMax=3 -N -R 0.0.0.0:8080:localhost:8000 centos_user@centos_ip

# 将上述命令保存到taskscheduler, 开机自动启动
```

```bash
# 第3台PC访问: curl http://centos_ip:8080/
```

## networkx

可以通过networkx计算相关性节点数量
> `pip install networkx`

```py
import polars as pl
from itertools import combinations
import networkx as nx

df = pl.scan_csv("etf_info.csv").filter(pl.col("type") == "bond").collect()

bond_index_list = df["tracking"].unique().to_list()

df_quote = pl.scan_ipc("tracking.ipc").filter(pl.col("index").is_in(bond_index_list)).collect()

df_bond_cv = df_quote.pivot(on="index", index="dt", values="close").sort("dt")
dfs = []
for pair_first, pair_second in combinations(df_bond_cv.columns[1:], 2):
    df = (
        df_bond_cv.select(pair_first, pair_second)
        .drop_nulls()
        .with_columns(
            pl.col(pair_first).pct_change(n=1),
            pl.col(pair_second).pct_change(n=1),
        )
        .select(
            pl.lit(pair_first).alias("first"),
            pl.lit(pair_second).alias("second"),
            pl.corr(pair_first, pair_second, method="pearson").alias("pearson"),
            pl.corr(pair_first, pair_second, method="spearman").alias("spearman"),
        )
    )
    dfs.append(df)
df_summary = pl.concat(dfs)

ratio = 0.85
df_corr = df_summary.filter(pl.col("spearman") > ratio)

G = nx.Graph(df_corr.select("first", "second").rows())
connected = list(nx.connected_components(G))
connection_flat = [item for s in connected for item in s]

connected

df_summary.filter(pl.col("first") == "950113.SH").filter(pl.col("pearson") > 0.9)

df_corr.write_csv('xxx.csv')
```