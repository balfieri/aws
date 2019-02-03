Some simple Amazon AWS scripts.  Written in Perl, mostly less than 10 lines of code.

<h1>One-Time Initialization</h1>

<h3>Set Up Your Local PC for AWS Command Line</h3>

<pre>
# clone/download this repo and put it on your PATH
cd [somewhere]
git clone https://github.com/balfieri/aws
export PATH=...

[install aws-cli (search web for how to do this on your type of PC)]

# configure your AWS environment
aws configure                           
[get AWS key id and secret id credentials from your AWS console and type them in when prompted]
region: us-east-1                       [or wherever suits you]

# some sanity checks
owner_group                             [will return your sg-nnn security group id]
owner_vpc                               [will return your vpc-nnn VPC id]
owner_region                            [will return the region you specified above]
</pre>

<p>You'll notice that those don't print a newline.  That's so other scripts can use them on command lines like `owner_group` etc.</p>

<h3>Create a SSH Key Pair</h3>

<p>
<pre>
# embed your LASTNAME in the key name or whatever you want to call it to make it unique
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' \
                        --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem

</pre>

<pre>
# allow TCP to communicate with your security group so you can ssh in etc.
auth_tcp_ingress
</pre>

<h3>Create Your "Master" Instance</h3>

<p>
Typically, you'll want one instance to act as a template for others.  We'll call this your "master instance."
Typically you'll get a program running on the master instance, then take a snapshot and use that snapshot to clone new instances.
More on that later.
</p>

<p>
The first one is slightly more complicated than doing others, but we need only do this once:
</p>

<pre>
# pull the most recent Amazon Linux image out of images.txt
# for me, it was ami-009d6802948d06e52
aws ec2 describe-images --owners amazon --filters 'Name=name,Values=amzn2-ami-hvm-2.Values=available' \
                        --output json > images.txt

# create one on-demand instance using the t2.medium instance type for starters
create_inst t2.medium ami-009d6802948d06e52 awsLASTNAMEkey

# sanity check
owner-insts                             [will return your i-nnn instance id]
</pre>

<p>
The next time you want to create an on-demand instance of the same type, you can just say:</p>

<pre>
create_inst
</pre>

<h3>Create Your Default get_instance Script</h3>

<p>
Create a "get_instance" executable(!) script in a directory that is on your PATH (this dir is fine!) and have it contain this code:</p>

<pre>
echo -n "i-nnn";                          # your i-nnn id returned by owner-insts
</pre>

<p>
Now when you type "get_instance" it will return your master instance id.
This is IMPORTANT because many of the scripts will use get_instance to get the
default instance id for operations.  You can usually override it, but usually you don't want to.
</p>

<h3>Install Software On Your Master Instance</h3>

<p>
SSH into the master instance and install apps that aren't there that you might need, such as C++:
</p>

<pre>
on_inst                                 [should ssh you to your master instance]
$ sudo yum update -y                    [updates system software]
$ sudo yum install gcc-c++              [or whatever apps you want]
$ exit                                  [logout]
</pre>

<h3>Stop Your Master Instance!</h3>

<p>Remember to stop your master instance when you aren't using it:</p>

<pre>
stop_inst
</pre>

<p>
Most of the scripts here will automatically issue a "start_inst" to make sure the instance is
started, but they won't stop the instance automatically unless the script requires that
it be stopped.</p>

<h1>Instance Queries</h1>

<p>
These commands take an instance id as an argument, but normally you'll supply no argument and
just let it use `get_instance` to get your master instance id.</p>

<pre>
inst_state                              [running, stopped, etc.]
inst_json                               [full instance info in JSON format]
 
inst_type                               [t2.medium, etc.]
inst_host                               ["" if not running, else the hostname it's running on]
inst_group                              [sg-nnn group id of instance]
inst_vol                                [volume id of root EBS root volume]
inst_device                             [/dev/xda1 or whatever root mount point]
inst_key                                [your awsLASTNAMEkey name]
inst_owner                              [your owner id]
inst_subnet                             [subnet id]
inst_vpc                                [VPC id]
inst_zone                               [us-east-1a, etc.]
</pre>

<h1>Instance Actions</h1>

<p></p>
<pre>
start_inst                              [scripts call this automatically, so don't need to manually]
stop_inst                               [stop instance if it's running]
change_inst_type type                   [stops instance and changes its type (t2.medium, etc.)]
</pre>

<pre>

on_inst                                 [ssh to the instance]
on_inst cmd ...                         [ssh to the instance and run "cmd ..."]
to_inst dst_dir src_files               [scp src_files to dst_dir on instance]
from_inst src_file dst_dir              [scp src_file from instance to dst_dir on this machine]
</pre>
