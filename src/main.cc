#include <stdlib.h>
#include <stdio.h> 
#include <cmath>    
#include <fstream>
#include "server/server.h"
#include "Poco/Exception.h"
#include "util/CmdLineParser.h"
#include "util/Exception.h"
#include "util/Util.h"
#include "pee.h"
#include "pep.h"

//TODO fazer comentários; passar para inglês

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

int read( const std::string& dataset, float**& input, int &ncol, int& nlin )
{
   std::ifstream infile( dataset.c_str() );
   std::string line; std::string token;
   bool firstline = true; bool iscomment = false;
   nlin = 0;
   while(std::getline(infile, line)) 
   {
      std::istringstream iss( line );

      int j = 0;
      while( std::getline(iss, token, ',') )
      {
         if( token[0] == '#' ) { iscomment = true; break; } 
         j++;
      }
      if( !firstline && !iscomment )
      {
         if( ncol != j )
         {
            fprintf(stderr,"Line '%d' has '%d' columns but the expected number is '%d'.\n", nlin+1, j, ncol);
            return 1;
         }
      }
      if( firstline && !iscomment ) { firstline = false; }
      if( iscomment ) { iscomment = false; } 
      else { ncol = j; }
      nlin++;
   }

   input = new float*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new float[ncol];

   infile.clear();
   infile.seekg(0);

   int i = 0; float tmp;
   firstline = true; iscomment = false;
   while(std::getline(infile, line, '\n')) 
   {
      std::istringstream iss( line );

      int j = 0;
      while( std::getline(iss, token, ',') )
      {
         if( firstline )
         {
            firstline = false;
            if ( !util::StringTo<float>(tmp, token) ) { iscomment = true; break; }
         }

         if( token[0] == '#' ) { iscomment = true; break; } 

         if ( !util::StringTo<float>(input[i][j], token)) 
         {
            fprintf(stderr, "Invalid input at line %d, column %d.\n", i+1, j+1);
            return 2;
         }
         if( isnan(input[i][j]) || isinf(input[i][j]) )
         {
            fprintf(stderr, "Invalid input at line %d, column %d.\n", i+1, j+1);
            return 2;
         }
         j++;
      }
      if( iscomment ) { iscomment = false; }
      else { i++; }
   }
   return 0;

   //if( scanf(token.c_str(),"%f%*[^\n],",&input[i][j]) != 1 || isnan(input[i][j]) || isinf(input[i][j]) )
   //if( scanf(token.c_str(),"%f,",&input[i][j]) != 1 || isnan(input[i][j]) || isinf(input[i][j]) )
}

void destroy( float** input, int nlin )
{
   for( int i = 0; i < nlin; ++i )
     delete [] input[i];
   delete [] input;
}


int main(int argc, char** argv)
{
   try {
      CmdLine::Parser Opts( argc, argv );

      Opts.Int.Add( "-port", "--number_of_port" );
      Opts.String.Add( "-d", "--dataset" );
      Opts.String.Add( "-sol", "--solution" );

      Opts.Process();

      Common::SetupLogger( "information" );

      int nlin; int ncol;
      float** input;


      int error = read( Opts.String.Get("-d"), input, ncol, nlin );
      if ( error ) {return error;}

      if( Opts.String.Found("-sol") )
      {
         pep_init( input, nlin, ncol, argc, argv );
         pep_interpret();
         pep_print( stdout );
         pep_destroy();
      }
      else
      {
         ServerSocket svs(SocketAddress("0.0.0.0", Opts.Int.Get("-port")));
         TCPServerParams* pParams = new TCPServerParams;
         //pParams->setMaxThreads(4);
         //pParams->setMaxQueued(4);
         //pParams->setThreadIdleTime(1000);
         TCPServer srv(new TCPServerConnectionFactoryImpl<Server>(), svs, pParams);
         srv.start();
         //sleep(100);

         pee_init( input, nlin, ncol, argc, argv );
         int generations = pee_evolve();
         fprintf(stdout, "\n> Overall best:");
         pee_print_best( stdout, generations, 1 );
         pee_print_time();
         pee_destroy();
      }

      destroy(input, nlin);
   }
   catch( const CmdLine::E_Exception& e ) {
      std::cerr << e;
      return 1;
   }
   catch( const Exception& e ) {
      std::cerr << e;
      return 2;
   }
   catch( const Poco::Exception& e ) {
      std::cerr << '\n' << "> Error: " << e.displayText() << " [@ Poco]\n";
      return 4;
   }
   catch( const std::exception& e ) {
      std::cerr << '\n' << "> Error: " << e.what() << std::endl;
      return 8;
   }
   catch( ... ) {
      std::cerr << '\n' << "> Error: " << "An unknown error occurred." << std::endl;
      return 16;
   }

   return 0;
}
