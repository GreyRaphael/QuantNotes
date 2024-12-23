# Task

- [Task](#task)
  - [crontab](#crontab)

## crontab

```bash
# list cron tasks
crontab -l

# edit cron tasks
crontab -e

# run at 09:20:00
20 9 * * * cd ~/rust_example/bktrader/src/python && ~/envs/duck/bin/fastapi run web.py

# run at reboot
@reboot cd ~/rust_example/bktrader/src/python && ~/envs/duck/bin/fastapi run web.py
```