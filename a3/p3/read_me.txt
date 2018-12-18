read_me.txt

EDIT: Dec/11/2018
I made modifications to my submission. My modifications are as follows:
- Fixed a bug in diskinfo, which was displaying an incorrect number of files.
- Implemented diskput.
Some other minor modifications were made that I noticed after testing
between the different programs (diskinfo, disklist, diskget, and diskput). For
that reason, my code might differ slightly from the one I originally submitted.
This submission should handle possible bugs in my original submission as well.

Some files were included to perform testing on the disk images (which are also
included). One file (Plini - Electric Sunrise Tab.pdf) tests if diskput rejects
a file larger than the free space available. The other files are the ones
included in the disk images.

=====

To compile the programs, simply type "make" in the command line, and
compilation will begin. To delete executables, enter "make clean".
The Makefile will generate 3 executables: diskinfo, disklist, and diskget.
My submission is missing diskput. Please ignore the warnings generated (if any),
I have noticed they occur because of a comparison to a certain number (0xE5).
