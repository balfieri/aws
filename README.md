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
owner_id                                # returns your owner (account) id (an integer)
owner_group                             # returns your sg-nnn security group id
owner_vpc                               # returns your vpc-nnn VPC id
owner_region                            # returns the region you specified above
</pre>

<p>You'll notice that those don't print a newline.  That's so other scripts can use them on command lines like `owner_group` etc.</p>

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
owner_insts
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
inst_state                              # running, stopped, etc.
inst_json                               # full instance info in JSON format
owner_insts                             # list i-nnn ids of all of your instances 
owner_insts_state                       # list i-nnn ids and state of all of your instances
owner_insts_json                        # list all information for all instances in JSON format
 
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
inst_zone                               # us-east-1a, etc.
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
owner_image                             # get ami-nnn id of most recently created image
owner_images                            # get ami-nnn ids of all created images
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

<p>
Create 5 on-demand instances that start by running a command_line.
Note: "command_line" is the run line ON the instance, NOT on the PC.</p>

<pre>
create_insts 5 -command "command_line"   
create_insts 5 -command "command_line" -type m3.medium 
</pre>

<p>
Create 5 on-demand instances cloned from the snapshot-nnn id.
</p>
<pre>
create_insts 5 -command "command_line" -clone snapshot-nnn      
</pre>

<p>
Create 5 on-demand instances cloned from the master.
This will first execute image_snapshot_inst above to clone the master:</p>
</p>
<pre>
create_insts 5 -command "command_line" -clone_master
</pre>

<p>
Create 5 spot instances with max spot price of $0.01/inst-hour:</p>

<pre>
create_insts 5 -command "command_line" -spot 0.01               
</pre>

<p>Create 5 spot instances cloned from the snapshot-nnn id:</p>

<pre>
create_insts 5 -command "command_line" -spot 0.01 -clone snapshot-nnn
</pre>

<p>
Create 5 spot instances cloned from the current master_inst:

<pre>
create_insts 5 -command "command_line" -spot 0.01 -clone_master
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
Here are the commands for deleting instances.  Refer to the next section
for how these are typically used.</p>

<pre>
delete_inst i-nnn
delete_inst i-nnn i-mmm ...
</pre>

<h1>On Each Launched Instance</h1>

<p>
The command script on each launched instance can use the following ec2-metadata commands to 
retrieve information it needs in order to figure out what work it should do.</p>

<pre>
ec2-metadata -d         # get command line
ec2-metadata -l         # get launch index
</pre>

<p>
The launch index is used to calculate which part of a larger job that
this instance is supposed to perform.</p>

<p>
When the instance is done with its work, it will typically issue a "shutdown" command, though
you can do anything you want.</p>

<h1>Retrieving Results from Instances</h1>

<p>
You can issue the following commands from your PC to get results from your instances and 
then delete (terminate) them.</p>

<p>
Return all i-nnn instance ids for all stopped instances
that were running the given command_line:</p>
<pre>
owner_insts -command "command_line" -state stopped
</pre>

<p>
For each stopped instance your PC-side script will typically
want to copy some results_file from the instance to the current directory
on your PC, and then delete the instance:</p>

<pre>
fm_inst i-nnn results_file .
delete_inst i-nnn
</pre>

<p>
You may also want to know if any other instances are still running,
though you would normally discern this from the number of
results that you have harvested so far:
</p>
<pre>
owner_insts -command "command_line" -state "pending|running"
</pre>

<p>
When you think all work is done and all results have been copied to your PC,
then you may or may not want to make sure all instances are terminated.
This command will return an empty string once that is true.  
</p>
<pre>
owner_insts -command "command_line" -state "!terminated"
</pre>

Bob Alfieri<br>
Chapel Hill, NC
