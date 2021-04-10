# riffdump
A cli-tool for inspecting content &amp; metadata of files compliant to the RIFF container format. 

**Disclaimer:** This software is part of a larger project I've been working on in my free time. I threw it together on short notice and I intend to patch additional functionality as time goes on. My experience with the C programming language is very limited and this software is mostly intended to be a fun and rewarding programming exercise. If you intend to use this software, please do so with caution. It's ~~probably~~ *likely* terribly written and you should keep that in mind.
## Installation
## Usage
### Examples
The following examples illustrate the most common use cases and serve as a short introduction to the software.

`riffdump [file]` - Retreives the extension identifier of a given RIFF-based file. This should describe the actual content type of the leading RIFF chunk that is the root of all subchunks. ~~Fails if the file is not recognized as a valid implementation of RIFF.~~  

`riffdump [file] -c` - Counts the number of subchunks that are **direct descendants** of the leading RIFF chunk. Does not count subchunks of subchunks (i.e. if a LIST subchunk is present).

`riffdump [file] -l` - Lists all subchunks that are **direct descendants** of the leading RIFF chunk. Does not list subchunks of subchunks (i.e. if a LIST subchunk is present). Subchunks are identified by their respective FourCC identifier.

`riffdump [file1] [file2] [...]` - Retreives the extension identifier of **all** given RIFF-based files. Processing multiple files at once is supported but not always recommended as this potentially removes explicit interpretations for the output. Readability is key, never sacrifice it.

`riffdump -v ...` - Produces verbose output. This works with most options and improves readability for humans. In general, verbose output is formatted and might include additional metadata. 
