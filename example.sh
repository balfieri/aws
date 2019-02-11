#!/bin/bash
#
# On each instance, stdout is written to /var/log/cloud-init-output.log
#
ec2-metadata -d
ec2-metadata -l
echo "PASS"
