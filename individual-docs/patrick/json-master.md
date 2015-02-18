
...

#!c++

#include <fstream>
#include <exception>

using namespace std;

#include "json/json.h"

extern "C" {
#include "evpath.h"
#include "ev_dfg.h"
}

typedef struct _s1 {
  char *s;
} message;

static FMField message_field_list[] =
  {
    { "string field", "string", sizeof(char*), FMOffset(message*, s) },
    { NULL, NULL, 0, 0 }
  };
static FMStructDescRec message_format_list[] =
  {
    { "message", message_field_list, sizeof(message), NULL },
    { NULL, NULL }
  };


static int
message_handler( CManager cm, void *vevent, void *client_data, attr_list attrs )
{
  message *event = std::reinterpret_cast< message* >( vevent );
  cout << "Hello, world!" << endl;
  return 1;
}
  
EVclient test_client;

int main (int argc, char *argv[])
{
  CManager cm;
  EVmaster master;
  EVdfg test_dfg;
  EVdfg_stone src, mid, sink;
  EVsource source_handle;
  Json::Value root;
  string master_name;
  
  try {
    std::ifstream doc( argv[1] /* JSON file name */, std::ifstream::binary );
    doc >> root;
  }
  catch ( std::exception& e ) {
    cerr << "std::exception while opening/parsing JSON file (" << argv[1] << ")" <<
	 << e.what()
	 << endl;
  }
    
  Json::Value nodes = root["nodes"];
  Json::Value edges = root["edges"];
  
  cm = CManager_create();
  CMlisten( cm );

  master = EVmaster_create( cm );
  char *contact = EVmaster_get_contact_list( master );

  EVclient_sources source_capabilities;
  EVclient_sinks sink_capabilities;

  source_handle = EVcreate_submit_handle( cm, -1, message_format_list );
  source_capabilities = EVclient_register_source( "event source", source_handle );
  sink_capabilities = EVclient_register_sink_handler
    ( cm, "message handler", message_format_list,
      reinterpret_cast<EVSimpleHandlerFunc>( message_handler ), NULL );

  test_dfg = EVdfg_create( master );
  
  /* get list of node names and build stone map 
   * 
   * multiple things going on here to avoid multiple traversals
   * of the node list. Premature optimization is the root of all evil.
   * I think Knuth said that.
   */
  std::vector<char*> nodenames;
  std::map<string, EVdfg_stone> stonemap;
  
  for (int i = 0; i < nodes.size(); ++i) {
    string name = nodes[i].get( "name" ).asString();
    nodenames.push_back( std::const_cast<char*>( name.c_str() ) );

    if (nodes[i].isMember( "master" )) master_name = name;

    EVdfg_stone s;
    if (nodes[i].get("source", true).asBool()) {
      /* this node thinks its a source */
      s = EVdfg_create_source_stone( test_dfg, "source" );
    }
    else if (nodes[i].get( "sink", true ).asBool()) {
      /* this node thinks its a sink */
      s = EVdfg_create_sink_stone( test_dfg, "message handler" );
    }
    else {
      /* regular old stone */
      s = EVdfg_create_stone( test_dfg, NULL );
    }

    EVdfg_assign_node( s, name );
    stonemap[name] = s;
  }

  nodenames.push_back( 0 ); /* satisfy EVpath null-terminated array fetish */
  EVmaster_register_node_list( master, nodenames.data() );

  /* Link the stones according to the edge list */
  for (int i = 0; i < edges.size(); ++i) {
    /* 
       get the source and target indices from the edge list.
       use them to index into the nodes list to get names
       use names to index the stonemap to get stone ids
       link the stones
    */
    int s, t;
    s = links[i].get( "source" ).asInt();
    t = links[i].get( "target" ).asInt();
    /* the indexes are 1-based */
    const string& ss = nodes[s-1].get( "name" ).asString();
    const string& ts = nodes[t-1].get( "name" ).asString();

    EVdfg_link_port( stonemap[ss], 0, stonemap[ts] );
  }

  EVdfg_realize( test_dfg );
  test_client =
    EVclient_assoc_local( cm, master_name, master, source_capabilities, sink_capabilities );

  cout << "Contact list is " << contact << endl;
  if (EVclient_ready_wait( test_client ) != 1) {
    /* DFG init failed */
    exit(1);
  }

  if (EVclient_source_active( source_handle )) {
    message m;
    m.s = "Hello, world";
    EVsubmit( source_handle, &m, NULL );
  }

  return EVclient_wait_for_shutdown( test_client );
}
...  







