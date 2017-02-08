# CurvHub

If Curv is to have an active community, then we'll need an easy way to
publish curv modules for others to use, and for others to find a published
module that meets some criteria.

Google isn't good enough, unfortunately. Some actual web infrastructure
is needed. A searchable online index of published Curv modules.

It should have characteristics of:
* shadertoy.com
* thingiverse.com
I.e., it's a place to publish and advertise cool shapes and parameterized
shapes that you have created. When you do a search, you see a picture of
each shape in the search results.

It should also have the characteristics of the online package repositories
associated with all of the top programming languages. It's a place to find
code libraries.

Curv does have a curated standard library that's built in to the
distribution. But this doesn't need to be huge. We probably want to avoid
the Python "stdlib is where packages go to die" phenomenon. OpenSCAD also has
some rather questionable built-in functions, which can never be fixed or
removed due to backward compatibility. So most Curv library code will
live in CurvHub, which is overall uncurated.

CurvHub requirements:
* It's a searchable index of 3rd party Curv modules.
* There's lots of metadata available:
  * A short name by which the module is referred to in conversation.
    (A valid Curv identifier?)
  * A globally unique hierarchical name which is used for referencing the
    module from within code. Not like Python `pip` where the package name and
    the module name can be different.
  * An icon or animated GIF.
  * a list of #tags or keywords,
  * a short 1-line description
  * a long description.
  * author
  * Licence. I want to restrict the licences to a dropdown list of
    open source licences which are mutually compatible. With a reasonable
    default.
* Popularity data. Eg, you may get a lot of search results, but you can filter
  on popularity. So how do we collect those metrics?
  * # of likes; registered users can click "Like" button.
  * # of views, on CurvHub
  * # of downloads, from CurvHub
  * # of references from other modules indexed by CurvHub
  * # of remixes (as reported by authors, as on Thingiverse)? It's better if
    remixes are done using references. But it's valid to copy and paste code
    from another module so you can rework it into something different.
  * # of collections that contain this module (like on Thingiverse)

IDE-like features:
  * You can click on a search result and view the source code online.
  * You can customize the parameters and view the results online,
    like Thingiverse.
  * You can modify the code and view the results online, like shadertoy.
  * You can use the web site like an IDE, create a new module, edit its code,
    test it, publish it.
  * Maybe we don't want CurvHub to completely duplicate all of the functions of
    github. You ought to be able to just link to a module that is hosted
    on github, or anywhere else for that matter, and do all of the actual
    development over there.
  * You can and publish a collection of CurvHub modules. This shows people what
    you like/endorse. It also makes these modules quickly available in your
    code editor when you are building a new module online.

Effects on the Curv language (what is a module):
 * Suppose I want to publish a 'voxelize' module, with a cute icon.
   It maps numeric parameters and a shape onto a minecrafty version
   of the shape. Other people should be able to refer to this by hierarchical
   name or url, add it to their collection.

Forum features?
 * You can post comments about a shape, like on shadertoy or thingiverse.
   Logged in users only. A "flag" button to report inappropriate comments.
   Staff members to monitor the flags and exert editorial control? Oops.

## Decentralization
How much of this can be decentralized/delegated? (Without forcing casual users
to set up their own servers.)
Maybe check out
* JoinDiaspora.com ("decentralized facebook")
* Synereo.com (vague and ambitious, decentralized Attention Economy network)
  Also, total vapourware right now.
