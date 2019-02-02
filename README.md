Trivial Amazon AWS scripts.  Written in Perl, mostly less than 10 lines of code.

<h1>One-Time Initialization</h1>

<h3>Get Stuff Setup on Your Local PC</h3>

<pre>
install aws-cli (search web for this)
aws configure
region: us-east-1                       [or whatever suits you]
[get credentials from your AWS console]

[put this directory on your PATH]
[now some sanity checks:]
owner_group                             [will return your sg-nnn security group id]
owner_vpc                               [will return your vpc-nnn VPC id]
owner_region                            [will return the region you specified above]

[Those don't print a newline so that you can use them on command lines like `owner_group` etc.]

[create a key pair for your LASTNAME (for me, awsalfierikey) or call it whatever you want:]
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem

[allow TCP to get to your security group:]
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
Create a get_instance executable script somewhere on your path (like this dir) and have contain this stuff:</p>

<pre>
cat > get_instance
#!/usr/bin/perl -w
#
print "i-nnn";                          [your i-nnn id returned by owner-insts]
^D
</pre>

<p>
Now when you type get_instance it will return your master instance id.
This is important because many of the script described next will use get_instance to get the
default instance id for operations.  You can often override it, but usually you don't want to.
</p>

<h3>Install Software On Your Master Instance</h3>

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
started.</p>

