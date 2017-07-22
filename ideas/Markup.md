How is my documentation written?
* markup language? markdown, reStructuredText or asciiDoctor
  * embedded math equations?
* use same markup language for inline documentation (embedded in Curv code)
  and standalone documentation;
* who is writing the documentation, and what are their needs?
  * me? a group effort, like the OpenSCAD wiki?
  * artists?  mathematicians? developers?
  * ease of use and learning curve?
* interface for editing Curv documentation.
  * can do it entirely in the web browser. No need to use git.
  * WYSIWYG editor would be nice.
* the documentation appears on the official Curv web site.
* documentation is printable? (Seems to be an issue for OpenSCAD.)
  * requires extra effort: need to linearize the hypertext.
    Need a transclusion directive?

Don't sweat the choice of markup language.
First question is: how do other people participate in editing the documentation?
* If I store the documentation in curv/docs, then:
  * It is versioned along with the code. To get docs for a particular release,
    just check out that release tag.
  * Only project members can edit the documentation. This is more restrictive
    than a wiki.
  * I trivially get an auto-generated web site using github pages.
* Suppose I want everyone with a github ID to be able to edit the docs.
  * Could use the github wiki. It will not auto-publish as a separate web site.
  * Could create a new public repository, curv-docs, with public write access.
    I'll have a directory tree structure.
  * A general issue is versioning.
