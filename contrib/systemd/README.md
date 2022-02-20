# Systemd integration example

This example contains my (yatli) personal configuration for sist2 auto-updating.
The following indices are involved in this configuration:

| Index     | Path             | Description                                |
|-----------|------------------|--------------------------------------------|
| files     | /zpool/files     | Main file repository                       |
| nextcloud | /zpool/nextcloud | Externally synchronized to a cloud account |

The systemd integration achieves automatic sist2 scanning & indexing everyday at 3:00AM.

### Tailoring the configuration for yourself

`sist2-update-all.sh` calls update scripts for each sist2 index. Add or remove
update scripts accordingly to suit your need. Each update script (e.g.
`sist2-update-files.sh`) has important parameters laid down at the beginning so
make sure to edit them to point to your files and index locations.

### Installation

```bash
# install the services and scripts
sudo make install
# enable & start the timer
sudo systemctl enable sist2-update.timer
sudo systemctl start sist2-update.timer
# verify that the timer has been enabled
systemctl list-timers --all
```

