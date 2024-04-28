#
# Generate initial data structures and header file for pre-defined tokens
#

$token_prefix = "BRT_";
$current = 1;

# Read all tokens into an array
#
open(TOK,"<$ARGV[0]");
while (<>) {

	# Ignore comments
	#
	next if(/^#/);

	# Look for a type definition
	#
	if(/^\$\s*(\w+)(?:\s+(\w+)\s+(\w+)?)?$/) {
		# Add type to type table
		#
		$name = "\U$1";

		$type_ext{$1} = $2;
		$type_tok{$1} = $name;
		$type_member{$1} = $3;

		# Create token for type
		#
		if(!$token_val{$name}) {
			$token_val{$name} = "none";
			$token_id{$name} = $current++;
		}

		next;
	}

	# Look for a token definition
	#
	if(/\s*(\w\w*)\s*(\w\w*)/) {
		die "Unknown type: $2" unless $type_tok{$2} ;

		$name = "\U$1$type_ext{$2}";
		$value = $2;

		if(!$token_val{$name}) {
			$token_val{$name} = $value;
			$token_id{$name} = $current++;
		}
	}
}
close(TOK);

#foreach $t (sort(keys %token_val)) {
#	print "$t = $token_val{$t} $token_id{$t} $token_len{$t}\n";
#}

# Header file
#
open(STDOUT,">pretok.h");

	print <<END;
/*
 * Copyright (c) 1993-1995 Argonaut Technologies Limited. All rights reserved.
 *
 * Predefined tokens generated via tokgen.pl
 */
#if BR_TOKEN_ENUM

typedef enum {
END

foreach $t (sort(keys %token_val)) {
	printf("	%-30s = %8s,\n",
		"$token_prefix$t",
		"$token_id{$t}");
}
	print <<END;
	_BRT_MAX = 0x7ffffff	/* Forces enum to be 32 bit */
} br_token;
#else
END

foreach $t (sort(keys %token_val)) {
	printf("#define %-30s %8s\n",
		"$token_prefix$t",
		"$token_id{$t}");
}

	print <<END;
#endif
END

# Spit out some macros for glueing a token base name to a type
#
foreach $t (sort(keys %type_tok)) {
	print("#define BRT_AS_$type_tok{$t}(tok)	BRT_##tok##$type_ext{$t}\n") if ($type_ext{$t});
}
close(STDOUT);

# Predefined table
#
open(STDOUT,">pretok.c");

# Special-case - the NULL token.
#
printf("\t[%-32s] = { .node = {0,}, .identifier = %-32s, .type = %-24s, .token = %-32s, .base_length = %8d},\n",
		"BR_NULL_TOKEN",
		"NULL",
		"BR_NULL_TOKEN",
		"BR_NULL_TOKEN",
		0);

foreach $t (sort(keys %token_val)) {
	$len = length($t) - length($type_ext{$token_val{$t}});

	printf("\t[%-32s] = { .node = {0,}, .identifier = %-32s, .type = %-24s, .token = %-32s, .base_length = %8d},\n",
		"$token_prefix$t",
		"\"$t\"",
		"$token_prefix$type_tok{$token_val{$t}}",
		"$token_prefix$t",
		$len);
}

print "#define NEXT_FREE_TOKEN $current\n" ;

close(STDOUT);

# Type table, sorted by suffix length, largest first
#
open(STDOUT,">toktype.c");

foreach $t (sort({length($type_ext{$b}) <=> length($type_ext{$a})} keys %type_tok)) {

	if($type_member{$t}) {
		$tsize = "sizeof(((br_value*)0)->$type_member{$t})";
	} else {
		$tsize = "0";
	}
	printf("\t{ .identifier = %-24s, .type = %-24s, .length = %8d, .size = %-32s},\n",
	    "\"$type_ext{$t}\"",
	    "$token_prefix$type_tok{$t}",
	    length($type_ext{$t}),
		$tsize
	);
}

close(STDOUT);
