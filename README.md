<p>
These are some trivial scripts that I use to do things on Amazon's cloud computing
platform, AWS.  AWS is confusing and it's hard to remember all the commands and operations that one
has to do.  These scripts attempt to make it simpler and to give a little structure and order
to the otherwise frustrating.  I use AWS for doing large, parallel simulations, so these scripts are
geared toward that kind of usage scenario.  
</p>

<p>
These scripts have no licensing restrictions whatsoever.  They are public domain.</p>

<h1>One-Time Setup</h1>

<h4>Set Up Your Local PC for AWS Command Line</h4>

<p>Clone or download this repo and put it on your PATH:</p>

<pre>
cd [somewhere]
git clone https://github.com/balfieri/aws
export PATH=...
</pre>

<p>
Install aws-cli (search web for how to do this on your type of PC).</p>

<p>Configure your AWS environment:</p>
<pre>
aws configure                           
[get AWS key id and secret id credentials from your AWS console and type them in when prompted]
region: us-east-1                       # or wherever suits you
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

<h4>Create an SSH Key Pair</h4>

<p>
Embed your LASTNAME in the key name or whatever you want to call it to make it unique.
This is just the convention that I happen to use:</p>
<pre>
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' \
                        --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem     # required
</pre>

<p>
Allow TCP to communicate with your security group so you can ssh in etc.:</p>
<pre>
auth_tcp_ingress
</pre>

<h4>Create Your "Master" Instance</h4>

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

<p>Create one on-demand instance using the t2.medium instance type for starters:</p>
<pre>
create_inst -type t2.medium -image ami-009d6802948d06e52 -key awsLASTNAMEkey
</pre>

<p>Sanity check to get your i-nnn instance id:</p>
<pre>
my_insts
</pre>

<h4>Create Your Default master_inst Script</h4>

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

<h4>Install Software On Your Master Instance</h4>

<p>
SSH into the master instance and install apps that aren't there that you might need, such as C++:
</p>

<pre>
on_inst                                 # should ssh you to your master instance
$ sudo yum update -y                    # updates system software
$ sudo yum install gcc-c++              # or whatever apps you want
$ exit                                  # logout
</pre>

<h4>Stop Your Master Instance!</h4>

<p>Remember to stop your master instance when you aren't using it.  This will perform the
equivalent of a "shutdown" on that instance.  But the EBS root volume persists.  We're not
deleting the instance.</p>

<pre>
stop_inst
</pre>

<p>Sanity check that's it's stopping:</p>
inst_state
</pre>

<p>
Most of the scripts here will automatically issue a "start_inst" to make sure the instance is
started, but they won't stop the instance automatically unless the script requires that
it be stopped.</p>

<h1>Instance Queries</h1>

<p>
These commands take an instance id as an argument, but normally you'll supply no argument and
just let it use `master_inst` to get your master instance id.</p>

<pre>
master_inst                             # i-nnn id of current master instance
inst_state                              # pending, running, shutting-down, terminated, stopping, stopped
inst_json                               # full instance info in JSON format
my_insts                                # list i-nnn ids of all of your instances 
my_insts -show_all                      # list all useful information for all instances
my_insts_json                           # list all information for all instances in JSON format
 
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
my_insts_zone                           # list i-nnn ids and the zone of all of your instances
</pre>

<h1>Other Owner Queries</h1>

<p>These commands are not specific to any instance(s):</p>
<pre>
my_id                                   # returns your owner (account) id (an integer)
my_group                                # returns your sg-nnn security group id
my_vpc                                  # returns your vpc-nnn VPC id
my_region                               # returns the region you specified above
my_regions                              # list of regions that your owner could use (only one allowed)
my_zones                                # list of availability zones within your owner region
</pre>

<h1>Instance Actions</h1>

<h4>Start/Stop/Modify</h4>
<pre>
start_inst                              # scripts call this automatically, so don't need to manually
stop_inst                               # stop instance if it's running
change_inst_type type                   # stops instance and changes its type (t2.medium, etc.)
resize_inst_vol gigabytes               # stops instance and resizes its root EBS volume
</pre>

<h4>SSH and SCP</h4>
<pre>
on_inst                                 # ssh to the instance
on_inst cmd args...                     # ssh to the instance and run "cmd args..."
to_inst src dst                         # scp src file or directory from this PC to dst on the instance
fm_inst src dst                         # scp src file or directory from instance to dst on this PC
</pre>

<h4>Snapshots</h4>
<pre>
snapshot_inst                           # take a snapshot of the instance's root volume (for backups)
vol_snapshot                            # get snapshot-nnn id of most recent snapshot for instance's root volume
vol_snapshots                           # get snapshot-nnn ids of all snapshots for instance's root volume
image_snapshot_inst                     # take a snapshot and create an ami-nnn launchable image from it 
my_image                                # get ami-nnn id of most recently created image
my_images                               # get ami-nnn ids of all created images
</pre>

<p>Note that snapshots do not consume extra space.  Amazon implements them using 
copy-on-write, so new space is allocated only when blocks are 
changed in one of the volumes (snapshot or other).</p>

<h1>Launching New Instances</h1>

<p>
Create one on-demand instance using master instance type, etc.  Note that
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

<p>
Create 3 on-demand instances that start by running a local_script.
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
#
# On each instance, stdout is written to /var/log/cloud-init-output.log
#
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

<h4>Delete Instances</h4>

<p>
Here are the commands for deleting instances:</p>

<pre>
delete_inst i-nnn
delete_inst i-nnn i-mmm ...
</pre>

<h1>On Each Launched Instance</h1>

<p>
The command script on each launched instance can use the following ec2-metadata commands to 
retrieve information it needs in order to figure out what work it should do.</p>

<p>
This command will retrieve the contents of the file:local_script:</p>
<pre>
ec2-metadata -d         
</pre>

<p>
This command will retrieve the launch index.
The launch index is used to calculate which part of a larger job that
this instance is supposed to perform.</p>
<pre>
ec2-metadata -l     
</pre>

<p>
When the instance is done with its work, it will typically write some kind of "PASS" indication
to stdout or some results_file in /tmp.</p>

<h1>Retrieving Results from Instances</h1>

<p>
You can issue the following commands from your PC to get results from your instances and 
then delete (terminate) them.</p>

<p>
The following command will show all useful information about all instances:</p>
<pre>
my_insts -show_all
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
delete_inst i-nnn
</pre>

<p>For the example.sh script earlier, we may do something as simple as:</p>
<pre>
on_inst i-nnn cat /var/log/cloud-init-output.log .
</pre>

<p>
That would print out something like this:</p>
<pre>
...
Cloud-init v. 18.2-72.amzn2.0.6 running 'modules:final' at Mon, 11 Feb 2019 23:12:33 +0000. Up 51.93 seconds.
user-data: #!/bin/bash          # ec2-metadata -d
#
# On each instance, stdout is written to /var/log/cloud-init-output.log
#
echo "script: `ec2-metadata -d`"
echo "launch id: `ec2-metadata -l`"
echo "PASS"
ami-launch-index: 0             # ec2-metadata -l
PASS
Cloud-init v. 18.2-72.amzn2.0.6 finished at Mon, 11 Feb 2019 23:12:33 +0000. Datasource DataSourceEc2.  Up 52.25 seconds
</pre>

<p>
Once all instances have been harvested, they should all be in the shutting-down or terminated state.  After about
15 minutes, terminated instances will drop off the <b>my_insts</b> list.</p>

Bob Alfieri<br>
Chapel Hill, NC
