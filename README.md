# Table of Contents

- [Overview](#overview)
- [One-Time Setup](#one-time-setup)
  - [My Security Philosophy](#my-security-philosophy)
  - [Set Up Your Local PC for AWS Command Line](#set-up-your-local-pc-for-aws-command-line)
  - [Create an Admin SSH Key Pair or Upload a Pre-Generated Public Key](#create-an-admin-ssh-key-pair-or-upload-a-pre-generated-public-key)
  - [Allow SSH Access to Instances](#allow-ssh-access-to-instances)
  - [Create Your "Master" Instance](#create-your-master-instance)
  - [Create Your Default master_inst Script](#create-your-default-masterinst-script)
  - [HIGHLY RECOMMENDED: Harden SSH On Your Master Instance](#highly-recommended-harden-ssh-on-your-master-instance)
  - [Install Software On Your Master Instance](#install-software-on-your-master-instance)
  - [Stop Your Master Instance!](#stop-your-master-instance)
- [Environment Queries](#environment-queries)
- [Environment Actions](#environment-actions)
- [Instance Queries](#instance-queries)
- [Instance Actions](#instance-actions)
  - [Start/Stop/Modify](#startstopmodify)
  - [SSH and SCP](#ssh-and-scp)
  - [Snapshots](#snapshots)
- [Launching New Instances](#launching-new-instances)
  - [Create One Instance](#create-one-instance)
  - [Create Multiple Instances](#create-multiple-instances)
  - [Create Multiple Instances as Clones of the Master and Execute a Script On Each](#create-multiple-instances-as-clones-of-the-master-and-execute-a-script-on-each)
  - [Create Multiple Spot Instances](#create-multiple-spot-instances)
- [On Each Launched Instance Running the Same Script](#on-each-launched-instance-running-the-same-script)
- [Harvesting Results from Instances Running the Same Script](#harvesting-results-from-instances-running-the-same-script)

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

1. Create a separate AWS (root) account for each group of users who need to work on the same stuff.  I prefer multiple accounts rather than one account with lots of complicated boundaries inside the account.  It's just less error-prone.  Use one Virtual Private Cloud (VPC) within each account. 

2. Create a special group within each AWS root account for admins of that account.  Call it "admins".  Only these users may perform administrative actions for the account.  Only they have SSH keys that allow them to SSH to instances as ec2-user  which should be the only sudo-capable Linux username.  

3. Use SSH as the main mechanism for connecting to instances.  Even VNC can be run through SSH tunnels.  I trust SSH when it's set up properly.  In a later section, I will show you how to harden SSH on your instances so that hackers have no chance of getting in.  For SSH, I use a Yubikey encryption device plugged into my PC with a required passcode, plus a time-based authenticator code on my phone.  I don't use VPN or special gateway instances.  Stick to one mechanism: SSH.  Do not even allow pinging of instances from the internet.

4. By default, do not allow non-admin users to do anything with AWS, including read-only things.  There is no reason to create some kind of "users" group by default.  Instead, allow those users only SSH access to one or more instances using non-admin Linux usernames.  For the most part, non-admin users can operate without even worrying about whether their servers live in AWS, Google Cloud, or on-premises.  Later, I will show how to map subdomain names to running instances to make it easier to log in in the face of changing IP addresses.

5. The exception to the default in #4 is when you want to allow users to launch their own instances.  In that case, you'll want to put them into their own security group within the VPC, possibly in a per-user security group.  This way, the user can administer his/her own instances within the VPC, and not have privilege to manipulate other instances in other security groups (besides SSH to those instances if you so allow).

6. Enable EBS volume encryption by default.  This ensures that all volumes (virtual disks) are encrypted by default.  Go to the EC2 Dashboard, select your region, then click on Settings under Account Attributes on the right.  Check the box labeled "Always encrypt new EBS volumes."  If you use multiple regions, you must do this for each region.</p>

## Set Up Your Local PC for AWS Command Line

<p>First you must obtain the "access key id" and "secret access key" from 
the AWS Console for your account.  As described earlier, you can have one "admins" security group
with you as the only user who can do anything, and then non-admin users having only SSH accesss.  Or you can decide to give non-admin users
their own security group(s) in which case you'd need to assign them access keys via the AWS console so that they can 
administer resources in that security group.  The difference between the "admins" group and these other non-admin groups and users is that 
the latter will likely have access to fewer resources and privileges within account.</p>

<p>
With that nuance (I hope) clarified, the rest of this document applies to any AWS user who has some level of 
administrative privilege within some security group within some VPC within some AWS account, even if that user is not
the AWS account owner.  For users who have
only SSH access to instances via normal SSH server mechanisms, they would not be allowed to execute any of the scripts described in this
document because they wouldn't have any AWS access keys.  They would have only SSH keys, which is a Linux server concept, not an AWS
concept.</p>

<p>
Install aws-cli.  Search web for how to do this on your type of PC.</p>

<p>Next, clone this repo and put it on your PATH:</p>

<pre>
cd [somewhere]
git clone https://github.com/balfieri/aws
export PATH=...
</pre>

<p>
Now we can configure the AWS environment on your PC which will set you up for a particular account, group, VPC, and region.</p>

<pre>
aws configure                           
                                        # then respond to the four questions: 
AWS Access Key ID:                      # from AWS Console
AWS Secret Access Key:                  # from AWS Console
Default region name:                    # e.g., us-east-1
Default output format:                  # I normally use "text"
</pre>

<p>Do some sanity checks:</p>
<pre>
my_account                              # returns your account/owner id (an integer)
my_group                                # returns your "sg-nnn" security group id
my_vpc                                  # returns your "vpc-nnn" VPC id (typically one per account)
my_region                               # returns the region you specified above
</pre>

<p>You'll notice that those don't print a newline.  That's so other scripts can use them on command lines like `my_group` etc.</p>

<p>By the way, if you want to list all regions that you have available to you, use this command, but keep in
mind that an ccount (owner) may belong to at most one region at a time:</p>
<pre>
my_regions                              # returns list of available regions
</pre>

## Create an Admin SSH Key Pair or Upload a Pre-Generated Public Key

<p>
We are going to create an key pair that we'll use to SSH to our master instance as ec2-user,
which is the only Linux username with sudo privilege.  
When we create the master instance, we'll
indicate that only this key pair is allowed to SSH into the instance.  
A little later, we'll add some non-admin users with their SSH keys, but we won't
use the commands shown here.  These are just for the admin ec2-user.

<p>
NOTE: ec2-user, as well as root and others,
will be initialized during aws instance creation to disallow password logins, so the only way to log in as ec2-user is using
SSH.  That's good because we don't want non-admin users to be able to "su - ec2-user".
Later, we will setting up non-admin users with the same restriction so that non-admin users can't "su - normal_user".</p>

<p>
Back to setting up SSH keys for you, the admin.
Embed your LASTNAME in the key name or whatever you want to call it to make it unique.</p>
<pre>
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' \
                        --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem     # required
</pre>

<p>
I prefer to generate my own SSH key pair and upload just the public key which is the only
key that's needed on the server side of SSH.
In fact, I use - and highly recommend - a Yubikey device which hides the private key even from me, 
so I couldn't upload the private key even if I wanted to.</p>

<pre>
aws ec2 import-key-pair --key-name awsLASTNAMEkey \
                        --public-key-material file://~/.ssh/awsLASTNAMEkey.pub
</pre>

## Allow SSH Access to Instances

<p>
Allow external users - including yourself - to SSH to instances in your security group.  SSH uses protocol TCP, port 22.  Without this authorization,
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
            "CidrIp": "0.0.0.0/0"        # allow any incoming IP
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
you (and only you) can SSH to the instance as admin ec2-user:</p>
<pre>
create_inst -type t2.medium -image ami-009d6802948d06e52 -key awsLASTNAMEkey
</pre>

<p>Note: the new EBS root volume will be encrypted assuming the account owner had set up default EBS volume encryption in the AWS
Console.</p>.

<p>Sanity check to get your i-nnn instance id:</p>
<pre>
my_insts
</pre>

## Create Your Default master_inst Script

<p>
Create a "master_inst" script in a directory on your PC that is on your PATH and have it contain this code where i-nnn is your instance id
shown by my_insts above:</p>

<pre>
#/bin/bash
echo -n "i-nnn";
</pre>

<p>
Now when you type "master_inst" it will return your master instance id.
<b>This is IMPORTANT because many of the scripts will use master_inst to get the
default instance id for operations.</b>  You can usually override it, but usually you don't want to.
</p>

<p>
Alternatively, assuming you have "." at the front of your PATH, you can have different master_inst scripts in different directories.
Then when you cd to a particular directory, these scripts will pick up the master_inst in that directory.
So the master_inst script gives context for most of the scripts described here.  Alternatively, you could write
one master_inst script and have it search up a directory tree until it finds the instance name in some
file you keep around.  It's up to you.  I use the former technique whenever possible.
</p>

## HIGHLY RECOMMENDED: Harden SSH On Your Master Instance

Coming soon...

## Install Software On Your Master Instance

<p>
First, update the existing software on your master instance:</p>
</p>

<pre>
on_inst sudo yum update -y 
</pre>

<p>
Install additional apps that you will need, such as C++ and Python3:</p>

<pre>
on_inst sudo yum install -y gcc-c++ python3 git
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

# Environment Queries

<p>These commands are not specific to any instance(s):</p>
<pre>
my_profile              # returns "default" (see next section)
my_account              # returns your account/owner id (an integer)
my_group                # returns your sg-nnn security group id
my_vpc                  # returns your vpc-nnn VPC id
my_region               # returns the region you specified above
my_regions              # list of regions that your owner could use (only one allowed)
my_zones                # list of availability zones within your owner region
</pre>

# Environment Actions

<p>When you configured your aws environment earlier, AWS gave your environment a profile name called "default".  
If you don't need more than one environment, then you can skip the rest of this section.</p>

<p>
If you'd like to set up a second profile, you can provide your own <b>my_profile</b> script that must be 
found earlier on your PATH than the one provided by this repo which always returns "default".  So let's
say you want to use a profile called "work1", you would create a <b>my_profile</b> script that does this:

<pre>
#!/bin/bash
echo -n "work1";
</pre>

<p>Test that it is found on your PATH before the default <b>my_profile</b> from this repo:</p>

<pre>
my_profile              # should return "work1", not "default"
</pre>

<p>Next, configure this profile the way you configured the "default" one, except supply the profile name
on the command line:</p>

<pre>
aws configure --profile work1
</pre>

<p>All subsequent scripts in this repo will now pick up your "my_profile" script and use "work1"
as the profile for all AWS commands.</p>

# Instance Queries

<p>
These commands show information for all instances:</p>

<pre>
master_inst             # i-nnn id of current master instance
my_insts                # list i-nnn ids of all of your instances 
my_insts -show_useful   # list useful information for all instances
my_insts_json           # list all information for all instances in JSON format
my_insts_zone           # list i-nnn ids and the zone of all of your instances
</pre>

<p>
These commands take an instance id as an argument, but normally you'll supply no argument and
just let it use `master_inst` to get your master instance id.</p>

<pre>
inst_state              # pending, running, shutting-down, terminated, stopping, stopped
inst_type               # t2.medium, etc.
inst_host               # "" if not running, else the hostname it's running on
inst_zone               # availability zone within region
inst_vpc                # VPC id
inst_subnet             # subnet id
inst_account            # owning account id of instance
inst_group              # sg-nnn group id of instance
inst_key                # creator's SSH key name
inst_image              # ami-nnn image id that the instance is running
inst_vol                # volume id of root EBS root volume
inst_device             # /dev/xda1 or whatever root mount point
inst_json               # list all information in JSON format
</pre>

# Instance Actions

## Start/Stop/Modify
<pre>
start_inst              # scripts call this automatically, so don't need to manually
stop_inst               # stop instance if it's running
change_inst_type type   # stops instance and changes its type (t2.medium, etc.)
resize_inst_vol gigabytes # stops instance and resizes its root EBS volume
</pre>

## SSH and SCP

<pre>
on_inst                 # ssh to the instance
on_inst cmd args...     # ssh to the instance and run "cmd args..."
to_inst src dst         # scp src file or directory from this PC to dst on the instance
fm_inst src dst         # scp src file or directory from instance to dst on this PC
</pre>

## Snapshots
<pre>
snapshot_inst           # take a snapshot of the instance's root volume (for backups)
vol_snapshot            # get snapshot-nnn id of most recent snapshot for instance's root volume
vol_snapshots           # get snapshot-nnn ids of all snapshots for instance's root volume
image_snapshot_inst name # take a snapshot and use it to create a new launchable image with name 
image_snapshot snapshot name # from an existing snapshot, create a new launchable image with name 
my_image                # get ami-nnn id of most recently created image
my_images               # get ami-nnn ids of all created images
my_vols                 # get vol-nnn ids of all created volumes (third column is true if encrypted)
</pre>

<p>Note that snapshots do not consume extra space.  Amazon implements them using 
copy-on-write, so new space is allocated only when blocks are 
changed in one of the volumes (snapshot or other).</p>

# Launching New Instances

## Create One Instance

<p>
Here's how to create one on-demand instance using master instance's image, type, etc.  Note that
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

## Create Multiple Instances as Clones of the Master and Execute a Script On Each

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
Remember that cloned volumes do not consume extra space until file system blocks
are modified.  Also, create_insts sets up the instances so that EBS root
volumes are deleted as their instances are deleted/terminated.</p>

<p>
My typical usage scenario for simulations is to copy a large amount of read-only data
to the master, then clone the master and have each instance produce a small
result file, so this cloning works out well.</p>

## Create Multiple Spot Instances 

<p>
Spot instances can be up to 80% less expensive than on-demand instances.
However, they could get terminated at any time by Amazon. I've never had this happen
because my instances typically run for less than one hour and I set my max spot price
to be the same as the on-demand price.  Still, there are no guarantees.
My recommendation is to just restart the whole job in this rare occurrence.</p>

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
Note that spot instances may not be stopped, but they can be terminated using
delete_inst.</p>

# On Each Launched Instance Running the Same Script

<p>
The script <b>running _on_ each launched instance</b> can use the following ec2-metadata commands to 
retrieve information it needs in order to figure out what work it should do.</p>

<p>
This command will retrieve the contents of the file:local_script:</p>
<pre>
$ ec2-metadata -d         
</pre>

<p>
This command will retrieve the launch index.
You can use the launch index to determine which part of your larger job that
this instance is supposed to perform. Its interpretation is completely up to you.</p>
<pre>
$ ec2-metadata -l         
</pre>

<p>
When the instance is done with its work, it will typically write some kind of "PASS" indication
to stdout or some results_file in /tmp.</p>

<p>
Of course, there are other ways each instance can get information.  It could look in an S3 bucket (more on that later) or use a 
database.  The above technique has an advantage that it doesn't require anything else besides the master instance.</p>

# Harvesting Results from Instances Running the Same Script

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

<p>Or gather up multiple names and use this one:</p>
<pre>
delete_insts i-nnn i-ooo i-ppp ...
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

<p>The work is done in this case, so our harvesting script might copy some files off the instance using fm_inst, then will
delete the instance:</p>
<pre>
delete_inst i-nnn
</pre>

<p>
Once all instances have been harvested, they should all be in the shutting-down or terminated state.  After about
15 minutes, terminated instances will drop off the <b>my_insts</b> list.</p>

Bob Alfieri<br>
Chapel Hill, NC
