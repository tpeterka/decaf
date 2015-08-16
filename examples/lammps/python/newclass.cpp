struct block
{
    int gid;            // global
    float bbox[6];      // global, relative to the data
    float temperature;  // global
    int num_particles;  // global, relative to the data
    float *pos;         // size = num_particules * 3
    float *speed;       // size = num_particules
}

int main()
{
    block_t b;

    //Creating the data field
    SimpleConstructData<int> gid( b.gid );
    BBoxConstructData bbox(b.bbox); // Reimplementation of VectorConstructData<float>
    SimpleConstructData<float> temperature( b.temperature );
    SimpleConstructData<int> num_particles( b.num_particles );
    VectorConstructData<float> pos( b.pos, 3 );
    VectorConstructData<float> speed( b.speed, 1 );

    //Filling the container
    ConstructData container;
    container.appendData(string("gid"), gid,
                         DECAF_NOFLAG, DECAF_SHARED,
                         DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
    container.appendData(string("bbox"), bbox,
                         DECAF_NOFLAG, DECAF_SHARED,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    container.appendData(string("temperature"), temperature,
                         DECAF_NOFLAG, DECAF_SHARED,
                         DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
    container.appendData(string("num_particles"), num_particles,
                         DECAF_NBITEMS, DECAF_SHARED,
                         DECAF_SPLIT_MINUS_NB_ITEMS, DECAF_MERGE_ADD_VALUE);
    container.appendData(string("pos"), pos,
                         DECAF_ZCURVE, DECAF_PRIVATE,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    container.appendData(string("speed"), speed,
                         DECAF_NOFLAG, DECAF_PRIVATE,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    vector<string> merge_order = {"gid", "temperature", "num_particles", "pos", "speed", "bbox"};
    container.setMergeOrder(merge_order);


   return 0;
