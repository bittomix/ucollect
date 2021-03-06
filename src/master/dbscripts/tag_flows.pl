#!/usr/bin/perl
use common::sense;
use DBI;
use Config::IniFiles;
use Fcntl ':flock';

my $lockfilename = "/tmp/tag_flows.lock";
open my $lockfile, '>>', $lockfilename or die "Could not open lock file '$lockfilename': $!\n";
flock $lockfile, LOCK_EX | LOCK_NB or exit;
my $cfg = Config::IniFiles->new(-file => $ARGV[0]);
shift @ARGV;

my %tags;

while (<>) {
	chomp;
	next if /^\s*(|#.*)$/; # Empty lines and comments
	next if /^\s*create\s/;
	if (my ($rule, $ip, $port) = /^\s*add\s+(\S+)\s+(\S+),(?:udp|tcp):(\d+)\s*$/) {
		$tags{$ip}->{ports}->{$port} = $rule;
		$tags{$ip}->{values} //= $rule; # Only if there wasn't a global one before
	} elsif (my ($rule, $ip) = /^\s*add\s+(\S+)\s+(\S+)\s*$/) {
		$tags{$ip}->{values} = $rule; # Override one from port-specific
	} else {
		die "Can't parse $_\n";
	}
}

my $dbh = DBI->connect("dbi:Pg:dbname=turris", "tagger", "", { RaiseError => 1, AutoCommit => 0 });
my $read = $dbh->prepare('SELECT id, ip_remote, port_remote FROM biflows WHERE tag IS NULL LIMIT 100000');
my $update = $dbh->prepare('UPDATE biflows SET tag = ?, tagged_on = ? WHERE id = ?');

my $anomalies = $dbh->prepare('SELECT DISTINCT value, type FROM anomalies WHERE relevance_count >= ?');
$anomalies->execute($cfg->val('anomalies', 'client_treshold'));
while (my ($ip, $type) = $anomalies->fetchrow_array) {
	$ip =~ s/(:|->)\d+//;
	next if lc $type eq 'p';
	$tags{$ip}->{values} = "anom-$type";
}

my $fake_blacklist = $dbh->prepare('SELECT server, remote FROM fake_blacklist');
$fake_blacklist->execute;
while (my ($server, $ip) = $fake_blacklist->fetchrow_array) {
	$tags{$ip}->{values} = "fake-$server";
}

my $found = 1;
my $count = 0;
while ($found) {
	my $batch = 0;
	my $tstamp = $dbh->selectrow_array("SELECT CURRENT_TIMESTAMP AT TIME ZONE 'UTC'");
	$read->execute;
	undef $found;
	while (my ($id, $ip, $port) = $read->fetchrow_array) {
		my $tag = $tags{$ip}->{ports}->{$port} // $tags{$ip}->{values} // '?';
		$update->execute($tag, $tstamp, $id);
		$found = 1;
		$count ++;
		$batch ++;
	}
	$dbh->commit;
	print "Done $count\n";
	last if $batch < 100000;
}

