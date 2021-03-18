# foo_openlyrics

An open-source lyrics plugin for [foobar2000](https://www.foobar2000.org/) that includes its own UI panel for displaying and sources for downloading lyrics that are not available locally.

## Why another lyrics plugin?
At the time that I started this, the most widely-used lyrics plugin was [foo_uie_lyrics3](https://www.foobar2000.org/components/view/foo_uie_lyrics3) which had several built-in sources but those had largely stopped working due to the relevant websites doing down or otherwise becoming generally unavailable. The original developer seemed to be nowhere in sight though and the source for the plugin did not appear to be available anywhere online. There is an SDK for building one's own sources for foo_uie_lyrics3 but building plugins for plugins didn't really take my fancy. Other (more up-to-date) plugins did exist but were mostly distributed by people posting binaries for you to download from their Dropbox on Reddit. Running binaries published via dropbox by random people on reddit did not seem like the most amazing idea.

As with most of my other projects, my response to this problem was (possibly ill-advisedly): How hard could it be to build my own one of these?

Maybe I'll be the only person that ever uses this. Maybe it'll become wildly popular and I'll make millions on donations. Maybe some people will use it and then I'll lose interest and stop maintaining it. No matter the outcome, the point of putting this here is that it will be available for people to use. If people don't like it and want to fork it, or if I stop maintaining it and all the sources break or whatever...people have access to the code and can make it work with less effort than I'm having to put in to build this from scratch. :)

## Status
The project has most of the major features implemented. Lyrics can be downloaded from a small number of locations on the internet as well as from a local directory (the same one used by foo_uie_lyrics3, so existing lyrics should just work if that was using it's default directory). There is also UI support for editing lyrics (including inserting timestamps). It is not as featureful or configurable as foo_uie_lyrics3 but it covers most of my (relatively simple) use-case for now.
