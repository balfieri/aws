# Table of Contents

- [Overview](#overview)
- [One-Time Setup](#one-time-setup)
  - [My Security Philosophy](#my-security-philosophy)
  - [Set Up Your Local PC for AWS Command Line](#set-up-your-local-pc-for-aws-command-line)
  - [Create an SSH Key Pair or Upload a Pre-Generated Public Key](#create-an-ssh-key-pair-or-upload-a-pre-generated-public-key)
  - [Allow SSH Access to Instances](#allow-ssh-access-to-instances)
  - [Enable EBS Volume Encryption by Default](#enable-ebs-volume-encryption-by-default)
  - [Create Your "Master" Instance](#create-your-master-instance)
  - [Create Your Default master_inst Script](#create-your-default-masterinst-script)
  - [Install Software On Your Master Instance](#install-software-on-your-master-instance)
  - [Stop Your Master Instance!](#stop-your-master-instance)
- [Instance Queries](#instance-queries)
- [Other Owner Queries](#other-owner-queries)
- [Instance Actions](#instance-actions)
  - [Start/Stop/Modify](#startstopmodify)
  - [SSH and SCP](#ssh-and-scp)
  - [Snapshots](#snapshots)
- [Launching New Instances](#launching-new-instances)
  - [Create One Instance](#create-one-instance)
  - [Create Multiple Instances](#create-multiple-instances)
  - [Delete Instances](#delete-instances)
- [On Each Launched Instance](#on-each-launched-instance)
- [Retrieving Results from Instances](#retrieving-results-from-instances)

# Overview

<p>
These are some trivial scripts that I use to do things on Amazon's cloud computing
platform, AWS.  AWS is confusing and it's hard to remember all the commands and operations that one
has to do.  These scripts attempt to make it simpler and to give a little structure and order
to the otherwise frustrating.  I use AWS for doing large, parallel simulations, so these scripts are
geared toward that kind of usage scenario.  
</p>

<p>
These scripts have no licensing restrictions whatsoever.  They are public domain.</p>

# One-Time Setup

## My Security Philosophy

I recommend the following:

1. Create a separate AWS (root) account for each GROUP of users who need to work on the same stuff.  I prefer multiple accounts rather than one account with lots of complicated boundaries inside the account.  It's just less error-prone.  Use one VPC within each account.  Keep it simple.

2. Use SSH as the main mechanism for connecting to instances.  Even VNC can be run through SSH tunnels.  I trust SSH.  In a later section, you will find instructions for setting up restrictive SSH on instances with multi-factor authentication.

3. Create a special group within each AWS root account for admins of that account.  Call it "admins".  Only these users may perform administrative actions for the account.  Only they have SSH keys that allow them to SSH to instances as "admin" - the only sudo-capable Linux username.  Do not allow root or ec2-user logins to instances.

4. Create a different group within each account for non-admin users.  These users may not perform any administrative chores, not even creating their own instances.  They may only SSH to existing instances using non-admin Linux usernames and associated SSH keys.  For people who need to be members of both admin and non-admin groups, make sure they use different user identities.  The same holds for people who need to be associated with multiple AWS root accounts.

## Set Up Your Local PC for AWS Command Line

<p>Clone or download this repo and put it on your PATH:</p>

<pre>
cd [somewhere]
git clone https://github.com/balfieri/aws
export PATH=...
</pre>

<p>
Install aws-cli (search web for how to do this on your type of PC).</p>

<p>Configure your AWS environment.  First you must obtain the "access key id" and "secret access key" from whomever manages
your AWS resources.  These are obtained from the AWS Console.</p>
<pre>
aws configure                           
                                        # then respond to the four questions: 
AWS Access Key ID:
AWS Secret Access Key:
Default region name:                    # e.g., us-east-1
Default output format:                  # I normally use text
</pre>

<p>Do some sanity checks:</p>
<pre>
my_id                                   # returns your owner (account) id (an integer)
my_group                                # returns your sg-nnn security group id
my_vpc                                  # returns your vpc-nnn VPC id
my_region                               # returns the region you specified above
</pre>

<p>You'll notice that those don't print a newline.  That's so other scripts can use them on command lines like `my_group` etc.</p>

<p>By the way, if you want to list all regions that you have available to you, use this command, but keep in
mind that an owner (account) may belong to at most one region at a time:</p>
<pre>
my_regions                              # returns list of available regions
</pre>

## Create an SSH Key Pair or Upload a Pre-Generated Public Key

<p>
We are going to create an initial key pair that we'll use to SSH to our master instance.  
When we create the master instance, we'll
indicate that only this key pair is allowed to SSH into the instance.  
Once we SSH into the master, we can manually set up new users and modify the 
/etc/ssh/sshd_config file to work for those users.  So this key pair
is temporary until the master instance is set up with real users and real SSH public keys.</p>

<p>
Embed your LASTNAME in the key name or whatever you want to call it to make it unique.
This is just the convention that I happen to use:</p>
<pre>
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' \
                        --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem     # required
</pre>

<p>
If you prefer to use your own key pair (or have a device like a Yubikey where the public key is generated by the device and the
private key is inaccessible), then you can upload your existing public key to AWS using something like the following.  Or you 
could add this to /etc/ssh/sshd_config on the master instance when you set up real users.</p>

<pre>
aws ec2 import-key-pair --key-name awsLASTNAMEkey \
                        --public-key-material file://~/.ssh/awsLASTNAMEkey.pub
</pre>


## Allow SSH Access to Instances

<p>
Allow external users to SSH to instances in your security group, in general.  SSH uses protocol TCP, port 22.  Without this authorization,
you won't even be able to get to the master instance even if you have a key pair set up above:</p>
<pre>
auth_group_ingress tcp 22             
</pre>

<p>
You may similarly set up egress rules using:

<pre>
auth_group_egress protocol port
</pre> 

<p>But I'd avoid that until you really need it.</p>

<p>
You can check your ingress and egress rules using this:</p>

<pre>
group_rules
</pre>

<p>That should include this ingress rule for ssh, among others:</p>

<pre>
                {
                    "PrefixListIds": [],
                    "FromPort": 22,
                    "IpRanges": [
                        {
                            "CidrIp": "0.0.0.0/0"
                        }
                    ],
                    "ToPort": 22,
                    "IpProtocol": "tcp",
                    "UserIdGroupPairs": [],
                    "Ipv6Ranges": []
                }
</pre>

<p>
You may later revoke ingress/egress access using one or both of following commands:</p>

<pre>
revoke_group_ingress protocol port

revoke_group_egress  protocol port
</pre>

## Enable EBS Volume Encryption by Default

<p>
This ensures that all volumes (virtual disks) are encrypted by default.  Go to the EC2 Dashboard, select your region, then click on
Settings under Account Attributes on the right.  Check the box labeled "Always encrypt new EBS volumes."  If you use multiple
regions, you must do this for each region.</p>

## Create Your "Master" Instance

<p>
Typically, you'll want one instance to act as a template for others.  We'll call this your "master instance."
Typically you'll get a program running on the master instance, then take a snapshot and use that snapshot to clone new instances.
More on that later.
</p>

<p>
The first one is slightly more complicated than doing others, but we need only do this once:
</p>

<p>Get the list of the most recent Amazon Linux2 images (assuming you want to run that).
This will return a list of ami-nnn image ids and creation dates.
Normally you'll just pick the most recent one:</p>
<pre>
linux2_images
</pre>

<p>If you just want to see the most recent one from Amazon, use this:</p>
<pre>
linux2_image
</pre>

<p>Create one on-demand instance using the t2.medium instance type for starters and the SSH key pair that you created above so that
you (and only you) can SSH to the instance:</p>
<pre>
create_inst -type t2.medium -image ami-009d6802948d06e52 -key awsLASTNAMEkey
</pre>

<p>Note: the new EBS root volume will be encrypted assuming you had set up default EBS volume encryption earlier.</p>.

<p>Sanity check to get your i-nnn instance id:</p>
<pre>
my_insts
</pre>

## Create Your Default master_inst Script

<p>
Create a "master_inst" script in a directory that is on your PATH and have it contain this code where i-nnn is your instance id:</p>

<pre>
#/bin/bash
echo -n "i-nnn";
</pre>

<p>
Now when you type "master_inst" it will return your master instance id.
This is IMPORTANT because many of the scripts will use master_inst to get the
default instance id for operations.  You can usually override it, but usually you don't want to.
</p>

<p>
Assuming you have "." at the front of your PATH, you can have different master_inst scripts in different directories.
Then when you cd to a particular directory, these scripts will pick up the master_inst in that directory.
So the master_inst script gives context for most of the scripts described here.  Alternatively, you could write
one master_inst script and have it search up a directory tree until it finds the instance name in some
file you keep around.  It's up to you.  I use the former technique whenever possible.
</p>

## Install Software On Your Master Instance

<p>
SSH into the master instance and install apps that aren't there that you might need, such as C++:
</p>

<pre>
on_inst                                 # should ssh you to your master instance
$ sudo yum update -y                    # updates system software
$ sudo yum install gcc-c++              # or whatever apps you want
$ exit                                  # logout
</pre>

## Stop Your Master Instance!

<p>Remember to stop your master instance when you aren't using it.  This will perform the
equivalent of a "shutdown" on that instance.  But the EBS root volume persists.  We're not
deleting the instance.</p>

<pre>
stop_inst
</pre>

<p>Sanity check that's it's stopping:</p>

<pre>
inst_state
</pre>

<p>
Most of the scripts here will automatically issue a "start_inst" to make sure the instance is
started, but they won't stop the instance automatically unless the script requires that
it be stopped.</p>

# Instance Queries

<p>
These commands show information for all instances:</p>

<pre>
master_inst                             # i-nnn id of current master instance
my_insts                                # list i-nnn ids of all of your instances 
my_insts -show_useful                   # list useful information for all instances
my_insts_json                           # list all information for all instances in JSON format
my_insts_zone                           # list i-nnn ids and the zone of all of your instances
</pre>

<p>
These commands take an instance id as an argument, but normally you'll supply no argument and
just let it use `master_inst` to get your master instance id.</p>

<pre>
inst_state                              # pending, running, shutting-down, terminated, stopping, stopped
inst_type                               # t2.medium, etc.
inst_host                               # "" if not running, else the hostname it's running on
inst_group                              # sg-nnn group id of instance
inst_image                              # ami-nnn image id that the instance is running
inst_vol                                # volume id of root EBS root volume
inst_device                             # /dev/xda1 or whatever root mount point
inst_key                                # your awsLASTNAMEkey name
inst_owner                              # your owner id
inst_subnet                             # subnet id
inst_vpc                                # VPC id
inst_zone                               # availability zone within region
inst_json                               # list all information in JSON format
</pre>

# Other Owner Queries

<p>These commands are not specific to any instance(s):</p>
<pre>
my_id                                   # returns your owner (account) id (an integer)
my_group                                # returns your sg-nnn security group id
my_vpc                                  # returns your vpc-nnn VPC id
my_region                               # returns the region you specified above
my_regions                              # list of regions that your owner could use (only one allowed)
my_zones                                # list of availability zones within your owner region
</pre>

# Instance Actions

## Start/Stop/Modify
<pre>
start_inst                              # scripts call this automatically, so don't need to manually
stop_inst                               # stop instance if it's running
change_inst_type type                   # stops instance and changes its type (t2.medium, etc.)
resize_inst_vol gigabytes               # stops instance and resizes its root EBS volume
</pre>

## SSH and SCP
<pre>
on_inst                                 # ssh to the instance
on_inst cmd args...                     # ssh to the instance and run "cmd args..."
to_inst src dst                         # scp src file or directory from this PC to dst on the instance
fm_inst src dst                         # scp src file or directory from instance to dst on this PC
</pre>

## Snapshots
<pre>
snapshot_inst                           # take a snapshot of the instance's root volume (for backups)
vol_snapshot                            # get snapshot-nnn id of most recent snapshot for instance's root volume
vol_snapshots                           # get snapshot-nnn ids of all snapshots for instance's root volume
image_snapshot_inst name                # take a snapshot and use it to create a new launchable image with name 
image_snapshot snapshot name            # from an existing snapshot, create a new launchable image with name 
my_image                                # get ami-nnn id of most recently created image
my_images                               # get ami-nnn ids of all created images
my_vols                                 # get vol-nnn ids of all created volumes (third column is true if encrypted)
</pre>

<p>Note that snapshots do not consume extra space.  Amazon implements them using 
copy-on-write, so new space is allocated only when blocks are 
changed in one of the volumes (snapshot or other).</p>

# Launching New Instances

## Create One Instance

<p>
Here's how to create one on-demand instance using master instance type, etc.  Note that
this does not clone the master instance's root drive, so this is normally used
to create a different master instance or miscellaneous instance:</p>

<pre>
create_inst                             
</pre>

<p>Override instance type:</p>
<pre>
create_inst -type t2.nano
</pre>

<p>Override availability zone (note: must be in my_region):</p>
<pre>
create_inst -zone us-east-1b
</pre>

## Create Multiple Instances

<p>
Here's how to create 3 on-demand instances that start by running a local_script.
The "local_script" is a script on your PC, not on the instance.  
<b>The local_script will be copied to each instance and is executed
as "root", not as "ec2-user".  The stdout of the script and other
stdout are written on the instance to /var/log/cloud-init-output.log, 
which is readable by ec2-user.</b>

<pre>
create_insts 3 -script "local_script"   
create_insts 3 -script "local_script" -type m3.medium 
</pre>

<p>
Here's an example script (example.sh) that simply executes two commands
that show the script (userdata) and the launch id of the instance, both
of which are described below, and then prints out "PASS":

<pre>
#!/bin/bash
ec2-metadata -d
ec2-metadata -l
echo "PASS"
</pre>

<p>
Create 3 on-demand instances cloned from the current master_inst.
This will first execute <b>image_snapshot_inst</b> above to clone the master:</p>
</p>
<pre>
create_insts 3 -script "local_script" -clone_master
</pre>

<p>
If you've already cloned the master, you can use the ami-nnn image id directly, which is available 
by first executing <b>my_image</b> to get the latest ami-nnn image created:</p>
<pre>
create_insts 3 -script "local_script" -image ami-nnn
</pre>

<p>
Create 3 spot instances with max spot price of $0.01/inst-hour:</p>

<pre>
create_insts 3 -script "local_script" -spot 0.01               
</pre>

<p>
Create 3 spot instances cloned from the current master_inst:

<pre>
create_insts 3 -script "local_script" -spot 0.01 -clone_master
</pre>

<p>
Remember that cloned volumes do not consume extra space until file system blocks
are modified.  Also, create_insts sets up the instances so that EBS root
volumes are deleted as their instances are deleted/terminated.</p>

<p>
My typical usage scenario for simulations is to copy a large amount of read-only data
to the master, then clone the master and have each instance produce a small
result file, so this cloning works out well.</p>

<p>
Note that spot instances can be up to 80% less expensive than on-demand instances.
However, they could get terminated at any time by Amazon. I've never had this happen
because my instances typically run for less than one hour and I set my max spot price
to be the same as the on-demand price.  Still, there are no guarantees.
My recommendation is to just restart the whole job in this rare occurrence.</p>

<p>
Also note that spot instances may not be stopped, but they can be terminated using
delete_inst.</p>

## Delete Instances

<p>
Here are the commands for deleting instances:</p>

<pre>
delete_inst i-nnn
delete_inst i-nnn i-mmm ...
</pre>

# On Each Launched Instance

<p>
The command script on each launched instance can use the following ec2-metadata commands to 
retrieve information it needs in order to figure out what work it should do.</p>

<p>
This command will retrieve the contents of the file:local_script:</p>
<pre>
[ec2-user@ip-...]$ ec2-metadata -d         
</pre>

<p>
This command will retrieve the launch index.
The launch index is used to calculate which part of a larger job that
this instance is supposed to perform.</p>
<pre>
[ec2-user@ip...]$ ec2-metadata -l         
</pre>

<p>
When the instance is done with its work, it will typically write some kind of "PASS" indication
to stdout or some results_file in /tmp.</p>

# Retrieving Results from Instances

<p>
You can issue the following commands from your PC to get results from your instances and 
then delete (terminate) them.</p>

<p>
The following command will show all useful information about all instances:</p>
<pre>
my_insts -show_useful
</pre>
<pre>
i-0fcb24869e6f081a1	2019-02-07T23:19:44.000Z	stopped	t2.medium	ami-009d6802948d06e52	us-east-1a	None	
i-0000de611bb6799db	2019-02-11T03:20:27.000Z	terminated	t2.medium	ami-013c046b8914ec5a7	us-east-1a	None	751871c6340f86b91aaf19478278b67c0af76a114cb88fe101b188c1fbda
i-0d0b236507c47ff02	2019-02-11T03:24:03.000Z	running	t2.medium	ami-013c046b8914ec5a7	us-east-1a	None	e12c1301f0cd35347ffa0c39f854e1354ad627aa5f4927a034a898d31b22
i-0aa0531f3b57fc14a	2019-02-11T03:24:03.000Z	running	t2.medium	ami-013c046b8914ec5a7	us-east-1a	None	e12c1301f0cd35347ffa0c39f854e1354ad627aa5f4927a034a898d31b22
i-01f1e5135fa9a2c0a	2019-02-11T03:24:03.000Z	running	t2.medium	ami-013c046b8914ec5a7	us-east-1a	None	e12c1301f0cd35347ffa0c39f854e1354ad627aa5f4927a034a898d31b22
</pre>

<p>
From that output, you can see that the last 3 were created at the same time.  That last long e12c130... number is supposed to be the 
-script string, but it's some other form of it, at least for spot instances.  It's better to go by the date.  We can
use this command from inside some script to get the i-nnn instance ids of those 3 and their states:</p>
<pre>
my_insts -time 2019-02-11T03:24:03.000Z -show_state
</pre>

<p>
Even easier, you could find all instances that were created using the last <b>my_image</b>:</p>
<pre>
my_insts -image `my_image` -show_state
</pre>

<pre>
i-0d0b236507c47ff02	running
i-0aa0531f3b57fc14a	running
i-01f1e5135fa9a2c0a	running
</pre>

<p>
For each running instance, your PC-side script will typically
want to copy some results_file or /var/log/cloud-init-output.log 
from the instance to the current directory
on your PC, and then delete the instance, assuming the instance is done
according to the results_file:</p>

<pre>
fm_inst i-nnn /tmp/results_file .
fm_inst i-nnn /var/log/cloud-init-output.log .
</pre>

<p>Make sure the work is done, then:</p>
<pre>
delete_inst i-nnn
</pre>

<p>For the example.sh script earlier, we could have done something as simple as the following to 
see the final result printed to stdout, which goes in /var/log/cloud-init-output.log on the
instance:</p>
<pre>
on_inst i-nnn cat /var/log/cloud-init-output.log
</pre>

<p>
That would print out something like the following.
Our "harvesting" script on the PC will grep that and see
that the work has been finished based on the "PASS" on the 2nd-to-last line, 
so it's ok to delete the instance:</p>
<pre>
...
Cloud-init v. 18.2-72.amzn2.0.6 running 'modules:final' at Mon, 11 Feb 2019 23:12:33 +0000. Up 51.93 seconds.
user-data: #!/bin/bash          # ec2-metadata -d
echo "script: `ec2-metadata -d`"
echo "launch id: `ec2-metadata -l`"
echo "PASS"
ami-launch-index: 2             # ec2-metadata -l
PASS
Cloud-init v. 18.2-72.amzn2.0.6 finished at Mon, 11 Feb 2019 23:12:33 +0000. Datasource DataSourceEc2.  Up 52.25 seconds
</pre>

<p>The work is done in this case, so our script might copy some files off the instance using fm_inst, then will
delete the instance:</p>
<pre>
delete_inst i-nnn
</pre>

<p>
Once all instances have been harvested, they should all be in the shutting-down or terminated state.  After about
15 minutes, terminated instances will drop off the <b>my_insts</b> list.</p>

Bob Alfieri<br>
Chapel Hill, NC
