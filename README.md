Amazon AWS scripts.  Written in Perl.  Documentation coming soon.

<h1>One-Time Initialization</h1>
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

[Those don't print a newline so that you can use them on commands lines `owner_group` etc.]

[create a key pair for your LASTNAME (for me, awsalfierikey) or call it whatever you want:]
aws ec2 create-key-pair --key-name awsLASTNAMEkey --query 'KeyMaterial' --output text > ~/.ssh/awsLASTNAMEkey.pem
chmod 400 ~/.ssh/awsLASTNAMEkey.pem

[allow TCP to get to your security group]
aws ec2 authorize-security-group-ingress --group-id `owner_group` --protocol tcp --port 22 --cidr 0.0.0.0/0 --region `owner_region`
</pre>

<h1>Create Your Master Instance</h1>

<pre>
aws ec2 describe-images --owners amazon --filters 'Name=name,Values=amzn2-ami-hvm-2.Values=available' --output json > images.txt
[look at images.txt and pick one, like ami-009d6802948d06e52]
