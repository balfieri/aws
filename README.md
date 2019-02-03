Trivial Amazon AWS scripts.  Written in Perl, mostly less than 10 lines of code.

<h1>One-Time Initialization</h1>

<h3>Get Stuff Setup on Your Local PC</h3>

<pre>
[put this directory on your PATH]

[install AWS command-line tool:]
[install aws-cli (search web for how to do this on your PC)]
aws configure
region: us-east-1                       [or whatever suits you]
[get credentials from your AWS console]

[now some sanity checks:]
owner_group                             [will return your sg-nnn security group id]
owner_vpc                               [will return your vpc-nnn VPC id]
owner_region                            [will return the region you specified above]
</pre>

<p>Those don't print a newline so that you can use them on command lines like `owner_group` etc.</p>

<h3>Create a Key Pair for SSH etc.</h3>

<p>
<pre>
[embed your LASTNAME in the key name or whatever you want to call it:]
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem

[allow TCP to get to your security group so you can login etc.:]
aws ec2 authorize-security-group-ingress --group-id `owner_group` --protocol tcp --port 22 --cidr 0.0.0.0/0 --region `owner_region`
</pre>

<h3>Create Your Master Instance</h3>

<p>
Typically, you'll want one instance to act as a template for others.  So you'll get a program running on the master instance,
then take a snapshot and use that snapshot to clone new instances.
</p>

<p>
The first one is a little more complicated than doing others:
</p>

<pre>
[do this to pick an ami-nnn image, look in image.txt and pick one that suits you:]
aws ec2 describe-images --owners amazon --filters 'Name=name,Values=amzn2-ami-hvm-2.Values=available' --output json > images.txt

[create this instance using t2.medium for starters; you can change the instance type later:]
aws ec2 run-instances --image-id ami-009d6802948d06e52 --count 1 --instance-type t2.medium --key-name awsLASTNAMEkey --security-group-ids `owner_group` --region  `owner_region`
owner-insts                             [will return your i-nnn instance id]
</pre>

<h3>Create Your Default get_instance Script</h3>

<p>
Create a "get_instance" executable script in a directory that is on your PATH (this dir is fine!) and have it contain this code:</p>

<pre>
#!/usr/bin/perl -w
#
print "i-nnn";                          # your i-nnn id returned by owner-insts
</pre>

<p>
Now when you type "get_instance" it will return your master instance id.
This is IMPORTANT because many of the script described next will use get_instance to get the
default instance id for operations.  You can often override it, but usually you don't want to.
</p>

<h3>Install Software On Your Master Instance</h3>

<p>
Log into the master instance and install apps that aren't there and you might need such as C++:
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
These commands take an instance id as an argument, but normally you'll just let it use "get_instance" to get
your master instance id.</p>

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
