<p>
These are some trivial scripts that I use to do things on Amazon's cloud computing
platform, AWS.  AWS is confusing and it's hard to remember all the commands and operations that one
has to do.  These scripts attempt to make it simpler and to give a little structure and order
to the otherwise frustrating.  I use AWS for doing large, parallel simulations, so these scripts are
geared toward that kind of usage scenario.  
</p>

<h1>One-Time Initialization</h1>

<h4>Set Up Your Local PC for AWS Command Line</h4>

<pre>
# clone/download this repo and put it on your PATH
cd [somewhere]
git clone https://github.com/balfieri/aws
export PATH=...

[install aws-cli (search web for how to do this on your type of PC)]

# configure your AWS environment
aws configure                           
[get AWS key id and secret id credentials from your AWS console and type them in when prompted]
region: us-east-1                       # or wherever suits you

# some sanity checks
owner_id                                # returns your owner (account) id (an integer)
owner_group                             # returns your sg-nnn security group id
owner_vpc                               # returns your vpc-nnn VPC id
owner_region                            # returns the region you specified above
</pre>

<p>You'll notice that those don't print a newline.  That's so other scripts can use them on command lines like `owner_group` etc.</p>

<h4>Create a SSH Key Pair</h4>

<p>
<pre>
# embed your LASTNAME in the key name or whatever you want to call it to make it unique
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' \
                        --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem     # required

</pre>

<pre>
# allow TCP to communicate with your security group so you can ssh in etc.
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

<pre>
# get list of most recent Amazon Linux2 images (assuming you want to run that)
# it will return a list of ami-nnn image ids and creation dates
# normally, you'll just pick the most recent one
linux2_images

# create one on-demand instance using the t2.medium instance type for starters
create_inst t2.medium ami-009d6802948d06e52 awsLASTNAMEkey

# sanity check
owner_insts                             # will return your i-nnn instance id
</pre>

<h4>Create Your Default master_inst Script</h4>

<p>
Create a "master_inst" executable(!) script in a directory that is on your PATH (this dir is fine!) and have it contain this code:</p>

<pre>
#/bin/bash
echo -n "i-nnn";                        # your i-nnn id returned by owner-insts
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
inst_state                              # verify that it went into the "stopping" state
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
owner_insts_json                        # list all information for all instances in JSON format
 
inst_type                               # t2.medium, etc.
inst_host                               # "" if not running, else the hostname it's running on
inst_group                              # sg-nnn group id of instance
inst_vol                                # volume id of root EBS root volume
inst_device                             # /dev/xda1 or whatever root mount point
inst_key                                # your awsLASTNAMEkey name
inst_owner                              # your owner id
inst_subnet                             # subnet id
inst_vpc                                # VPC id
inst_zone                               # us-east-1a, etc.
</pre>

<h1>Instance Actions</h1>

<p></p>
<pre>
start_inst                              # scripts call this automatically, so don't need to manually
stop_inst                               # stop instance if it's running
change_inst_type type                   # stops instance and changes its type (t2.medium, etc.)
</pre>

<pre>

on_inst                                 # ssh to the instance
on_inst cmd args...                     # ssh to the instance and run "cmd args..."
to_inst src dst                         # scp src file or directory from this PC to dst on the instance
fm_inst src dst                         # scp src file or directory from instance to dst on this PC
</pre>

<pre>

snapshot_inst                           # take a snapshot of the instance's root volume (for backups)
vol_snapshot                            # get snapshot-nnn id of most recent snapshot for instance's root volume
vol_snapshots                           # get snapshot-nnn ids of all snapshots for instance's root volume
image_snapshot_inst                     # take a snapshot and create an ami-nnn launchable image from it 
owner_image                             # get ami-nnn id of most recently created image
owner_images                            # get ami-nnn ids of all created images

# Note that snapshots do not consume extra space.  Amazon implements them using 
# copy-on-write, so new space is allocated only when blocks are 
# changed in one of the volumes (snapshot or other).

</pre>

<p>
<p>
<pre>
create_inst                             # create 1 on-demand instance using master instance type, etc.
create_inst t2.nano                     # same, but override instance type

</pre>

<pre>
# create 5 on-demand instances that start by running script_name 
# note: "command_line" is the run line ON the instance, NOT on the PC
create_insts 5 -command "command_line"   

# same, but override instance type
create_insts 5 -command "command_line" -type m3.medium                

# create 5 on-demand instances cloned from the snapshot-nnn id
create_insts 5 -command "command_line" -clone snapshot-nnn      

# create 5 spot instances with max spot price of $0.01/inst-hour
create_insts 5 -command "command_line" -spot 0.01               

# create 5 spot instances cloned from the snapshot-nnn id
create_insts 5 -command "command_line" -spot 0.01 -clone snapshot-nnn

# create 5 spot instances cloned from the current master_inst
# this will first execute image_snapshot_inst above to clone the master 
create_insts 5 -command "command_line" -spot 0.01 -clone_master
</pre>

<h1>On the Instance</h1>

<p>
A command script on each launched instance can use the following ec2-metadata commands to 
retrieve information it needs in order to figure out what work it should do:</p>

<pre>
ec2-metadata -d                         # the -command 'command_line' from create_insts above
ec2-metadata -l                         # launch index (0, 1, 2, ...)
</pre>

<p>
The launch index is typically used to calculate which part of a larger job that
this instance is supposed to perform.</p>

<p>
When the instance is done with its work, it will typically issue a "shutdown" command, though
you can do anything you want.</p>

<h1>Retrieving Results from Instances</h1>

<p>
You can issue the following commands from your PC to get results from your instances and 
then delete (terminate) them:</p>

<pre>
# return all i-nnn instance ids for all stopped instances
# that were running the given command_line
owner_insts -command "command_line" -state stopped

# for each stopped instance your PC-side script will typically
# want to copy some results_file from the instance to the current directory
# on your PC, and then delete the instance:
fm_inst i-nnn results_file .            # copy results_file to this PC
delete_inst i-nnn                       # delete inst and its root EBS 

# you may also want to know if any other instances are still running:
owner_insts -command "command_line" -state "pending|running"

# when you think all work is done and all results copied to your PC,
# then you may or may not want to make sure all instances are terminated
# this command will return an empty string once that is true
owner_insts -command "command_line" -state "not_terminated"
