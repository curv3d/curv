#!/bin/sh
#
# Usage: svx.sh [curv-source-file] [vsize]
# 

FILE=$1
VSIZE=$2
OUTDIR=${3:-"density"}

FDUR=0.01

DIMENSIONS=$(curv -x "file \"$FILE\" >> size")
WIDTH=$(echo "$DIMENSIONS"  | jq '.[0]')
HEIGHT=$(echo "$DIMENSIONS" | jq '.[1]')
DEPTH=$(echo "$DIMENSIONS"  | jq '.[2]')
TOTAL_DURATION=$(echo "scale=10 ; ($DEPTH / $VSIZE) * $FDUR" | bc)

BASENAME="slice"
DIRNAME=$(dirname "$FILE")
OUTPUT="$BASENAME"'-*';

XSIZE=$(echo "scale=10 ; $WIDTH  / $VSIZE" | bc )
YSIZE=$(echo "scale=10 ; $HEIGHT / $VSIZE" | bc )
ZSIZE=$(echo "scale=10 ; $DEPTH  / $VSIZE" | bc )
VSIZE_METERS=$(echo "scale=10 ; $VSIZE / 1000" | bc )

cd "$DIRNAME"

curv                         \
-o "$OUTPUT.png"             \
-O animate="$TOTAL_DURATION" \
-O fdur=$FDUR                \
-O xsize=$XSIZE              \
-O ysize=$XSIZE              \
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
{ shape=file \"$(basename $FILE)\", vsize=$VSIZE }";

mkdir -p "$OUTDIR";
mv "$BASENAME-"*".png" "$OUTDIR/";
for i in "$OUTDIR/$BASENAME-"*".png"
do
  convert $i -colorspace gray $i
done

echo "
<?xml version=\"1.0\"?>

<grid version=\"1.0\" gridSizeX=\"$WIDTH\" gridSizeY=\"$HEIGHT\" gridSizeZ=\"$DEPTH\"
   voxelSize=\"$VSIZE_METERS\" subvoxelBits=\"8\" slicesOrientation=\"Y\" >

    <channels>
        <channel type=\"DENSITY\" bits=\"8\" slices=\"$OUTDIR/$BASENAME-%03d.png\" />
    </channels>

    <material>
        <material id="1" urn="urn:shapeways:materials/1" />
    </material>

    <metadata>
        <entry key=\"author\" value=\"$USER\" />
        <entry key=\"creationDate\" value=\"$(date -I)\" />
    </metadata>
</grid>" > manifest.xml;

zip -r $(basename "$FILE" .curv).svx manifest.xml "$OUTDIR"/
rm -r manifest.xml "$OUTDIR"
