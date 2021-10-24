#!/bin/sh

FILE=$1
VSIZE=$2
OUTDIR=${3:-"slices"}

FDUR=0.01
DIMENSIONS=$(curv -x "file \"$FILE\" >> size")
WIDTH=$(echo "$DIMENSIONS" | echo "scale=10 ; $(jq '.[0]') / $VSIZE" | bc )
HEIGHT=$(echo "$DIMENSIONS" | echo "scale=10 ; $(jq '.[1]') / $VSIZE" | bc )
DEPTH=$(echo "$DIMENSIONS" | jq '.[2]')
TOTAL_DURATION=$(echo "scale=10 ; ($DEPTH / $VSIZE) * $FDUR" | bc)
BASENAME=$(basename "$FILE" ".curv")
DIRNAME=$(dirname "$FILE")
OUTPUT="$DIRNAME"'/'"$BASENAME"'-*';

curv                         \
-o "$OUTPUT.png"             \
-O animate="$TOTAL_DURATION" \
-O fdur=$FDUR                \
-O xsize=$WIDTH              \
-O ysize=$HEIGHT             \
-x "
(
  {vsize,shape} ->
  let
    fdur = $FDUR;
    zmin = shape.bbox.[MIN,Z];
  in make_shape {
    dist: [x,y,z,t] ->
      let
        frameno = floor(t / fdur);
        zz = zmin + vsize*frameno;
      in shape.dist[x,y,zz,0];
    colour: p -> white;
    is_2d: true;
    bbox: shape.bbox;
    render: {bg: black};
  }
)
{ shape=file \"$FILE\", vsize=$VSIZE }";

mkdir -p "$DIRNAME/$OUTDIR";
mv "$DIRNAME/$BASENAME-"*".png" "$DIRNAME/$OUTDIR/";
