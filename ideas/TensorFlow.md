TensorFlow™ is an open source software library for numerical computation
using data flow graphs. Nodes in the graph represent mathematical operations,
while the graph edges represent the multidimensional data arrays (tensors)
communicated between them. The flexible architecture allows you to deploy
computation to one or more CPUs or GPUs in a desktop, server, or mobile
device with a single API. TensorFlow was originally developed by researchers
and engineers working on the Google Brain Team within Google's Machine
Intelligence research organization for the purposes of conducting machine
learning and deep neural networks research, but the system is general enough
to be applicable in a wide variety of other domains as well.

TensorFlow and Curv are both tensor-based functional programming languages
that execute code on the GPU. Beyond that surface similarity, they are totally
different, but an integration into Curv would be possible, if it made sense.

https://www.tensorflow.org/

Magenta is a project use to use TensorFlow for procedurally generated
art and music, which seems to align with the goals of Curv.
But they now seem to focus solely on music.

https://magenta.tensorflow.org/

Lots of people doing 2D image manipulation using TensorFlow.
What about 3D solid modelling, where solids are represented by tensor fields?

Deep Dreaming: generating art using a neural network. You train a neural network
on images of trees, then run it backwards to generate tree images.
This is easy and accessible today.

Deep Curv: generating signed distance fields using a neural network.
You train a neural network on signed distance fields, then run it backwards.
Which SDF representations? Voxels. Curv programs.

Maybe we can use AI to search for optimized code for generating a Euclidean
SDF for a desired shape.

Generative Adversarial Networks (GAN): two networks trained on the same
family of shapes or images. One network generates candidates, the other network
accepts/rejects the candidates.
