Dependencies
===============

```
sudo apt-get install libgeoip-dev libglib2.0-dev libgdal-dev
```

About geolite-city
====================

geolite-city is not included in the debian package, but you can grab it from Maxmind, using:

```
wget -N http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz
gunzip GeoLiteCity.dat.gz
```

