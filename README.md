# log_aggregate_for_zabbix

purpose: aggregate url classified response staus from nginx log for zabbix monitor

## make binary (require boost)
clang++ -l boost_date_time log_report.cc -o log_report

## configuration step 
assume deploy to directory /data/monit:

### 1, set nginx log format like this, then reload nginx:

<pre>
        log_format json '{"remote_addr": "$remote_addr", "remote_user": "$remote_user", '
                '"time_iso8601": "$time_iso8601", "request": "$request", '
                '"status": $status, "body_bytes_sent": $body_bytes_sent, '
                '"request_length":  $request_length, '
                '"request_time": $request_time, "connection_requests": $connection_requests, '
                '"http_referer": "$http_referer", "http_user_agent": "$http_user_agent"}';

        access_log              /var/log/nginx/access_datacollect.log json;
</pre>

### 2, copy log_report discover.py to this directory

### 3, make a configuration file like this:

<pre>
{
    "log_file_name": "\/var\/log\/nginx\/access_datacollect.log",
    "last_position": "3101241185",
    "lock": "0",
    "urls": [
        "POST \/collect"
    ]
}
</pre>

here

log_file_name: point to nginx log file path.

last_position: initial it to 0 (begin of the file).

lock: initial it to 0 (indicate no instance run now).

urls: list urls you care here.


### 4, set cron routines like this:
<pre>
* * * * * /data/monit/log_report /data/monit/config.json /data/monit/log_stat.json > /dev/null 2>&1
</pre>

the log_stat.json should be like:

<pre>
{
    "ts": "2017-07-31T16:39",
    "POST \/collect:1xx:cnt": "0",
    "POST \/collect:1xx:avg": "0.00",
    "POST \/collect:2xx:cnt": "16012",
    "POST \/collect:2xx:avg": "22.73",
    "POST \/collect:3xx:cnt": "0",
    "POST \/collect:3xx:avg": "0.00",
    "POST \/collect:4xx:cnt": "122",
    "POST \/collect:4xx:avg": "29532.20",
    "POST \/collect:5xx:cnt": "0",
    "POST \/collect:5xx:avg": "0.00"
}    
</pre>

### 5, add two zabbix rules to zabbix-agent like this:

<pre>
UserParameter=nginx.log.requests, python3 /data/monit/discover.py /data/monit/config.json
UserParameter=nginx.log.requests.status[*], python3 -c 'import json ; print(json.loads(open("/data/monit/log_stat.json", "rt").read())["'"$1"'"])'
</pre>

### 6, set up zabbix server to create discovery rules to monitor request status...
