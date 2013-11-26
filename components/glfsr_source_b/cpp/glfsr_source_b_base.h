/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 * 
 * This file is part of GNUHAWK.
 * 
 * GNUHAWK is free software: you can redistribute it and/or modify is under the 
 * terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * GNUHAWK is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef GLFSR_SOURCE_B_IMPL_BASE_H
#define GLFSR_SOURCE_B_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Resource_impl.h>

#include "bulkio/bulkio.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include "glfsr_source_b_GnuHawkBlock.h"

#define NOOP 0
#define FINISH -1
#define NORMAL 1

class glfsr_source_b_base;

template < typename TargetClass >
class ProcessThread
{
    public:
        ProcessThread(TargetClass *_target, float _delay) :
            target(_target)
        {
            _mythread = 0;
            _thread_running = false;
            _udelay = (__useconds_t)(_delay * 1000000);
        };

        // kick off the thread
        void start() {
            if (_mythread == 0) {
                _thread_running = true;
                _mythread = new boost::thread(&ProcessThread::run, this);
            }
        };

        // manage calls to target's service function
        void run() {
            int state = NORMAL;
            while (_thread_running and (state != FINISH)) {
                state = target->serviceFunction();
                if (state == NOOP) {
                    boost::this_thread::sleep( boost::posix_time::microseconds( _udelay ) );
                } else {
                    boost::this_thread::yield();
                }
            }
        };

        // stop thread and wait for termination
        bool release(unsigned long secs = 0, unsigned long usecs = 0) {
            _thread_running = false;
            if (_mythread)  {
                if ((secs == 0) and (usecs == 0)){
                    _mythread->join();
                } else {
                    boost::system_time waitime= boost::get_system_time() + boost::posix_time::seconds(secs) +  boost::posix_time::microseconds(usecs) ;
                    if (!_mythread->timed_join(waitime)) {
                        return 0;
                    }
                }
                delete _mythread;
                _mythread = 0;
            }
    
            return 1;
        };

        virtual ~ProcessThread(){
            if (_mythread != 0) {
                release(0);
                _mythread = 0;
            }
        };

        void updateDelay(float _delay) { _udelay = (__useconds_t)(_delay * 1000000); };

        void stop() {
            _thread_running = false;
            if ( _mythread ) _mythread->interrupt();
        };

        bool threadRunning() { return _thread_running; };

    private:
        boost::thread *_mythread;
        bool _thread_running;
        TargetClass *target;
        __useconds_t _udelay;
        boost::condition_variable _end_of_run;
        boost::mutex _eor_mutex;
};

class glfsr_source_b_base : public GnuHawkBlock
{
    public:
        glfsr_source_b_base(const char *uuid, const char *label);

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        CORBA::Object_ptr getPort(const char* _id) throw (CF::PortSupplier::UnknownPort, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException);

        void loadProperties();

    protected:
        ProcessThread<glfsr_source_b_base> *serviceThread; 
        boost::mutex serviceThreadLock;

        // Member variables exposed as properties
        CORBA::Long degree;
        bool repeat;
        CORBA::Long mask;
        CORBA::Long seed;
        CORBA::ULong period;

        // Ports
        bulkio::OutOctetPort *byte_out;

        std::vector< std::string > outputPortOrder;

    private:
        void construct();

    protected:
        static       const        int                 RealMode=0;
        static       const        int                 ComplexMode=1;
        std::vector<std::vector<int> >                io_mapping;
        typedef      boost::posix_time::ptime         TimeMark;
        typedef      boost::posix_time::time_duration TimeDuration;
        typedef      BULKIO::PrecisionUTCTime         TimeStamp;

        //
        // Enable or disable to adjusting of timestamp based on output rate
        //  
        inline void maintainTimeStamp( bool onoff=false ) {
           _maintainTimeStamp = onoff;
        };

        //
        // Enable or disable throttling of processing
        //  
        inline void setThrottle( bool onoff=false ) {
           _throttle = onoff;
        };

        //
        // getTargetDuration
        //
        // Target duration defines the expected time the service function requires
        // to produce/consume elements. For source patterns, the data output rate
        // will be used to defined the target duration.  For sink patterns, the
        // input rate of elements is used to define the target duration
        //
        virtual TimeDuration getTargetDuration();

        //
        // calcThrottle 
        //
        // Calculate the duration about that we should sleep based on processing time
        // based on value from getTargetDuration() minus processing time ( end time 
        // minus start time)
        //
        // If the value is a positive duration then the boost::this_thread::sleep
        // method is called with 1/4 of the calculated duration.
        //
        virtual TimeDuration calcThrottle( TimeMark &stime,
                                           TimeMark &etime );

        //
        // createBlock
        //
        // Create the actual GNU Radio Block to that will perform the work method. The resulting
        // block object is assigned to gr_sptr
        //
        // Add property change callbacks for getter/setter methods
        //
        //
        virtual void createBlock() = 0;

        // 
        // setupIOMappings
        //
        // Sets up mappings for input and output ports and GnuRadio Stream indexes
        //
        // A Gnu Radio input or output streams will be created for each defined RedHawk port.
        // The streams will be ordered 0..N-1 as they are defined in inputPortOrder and outputPortOrder
        // lists created during Component initialization.  
        //
        // For Gnu Radio blocks that define -1 for maximum number of input streams. The number of
        // input streams created will be restricted to the number of RedHawk ports.
        //
        // RESOLVE - need to base mapping for -1 condition on "connections" and not streams
        // RESOLVE - need to add parameters to define expected modes for input ports.. i.e. real or complex and 
        //           not have to wait for SRI.
        //
        virtual void  setupIOMappings();

        //
        // getNOutputStreams
        //
        // Called by setupIOMappings when the number of Gnu Radio output streams == -1 (variable ) and number of 
        // Redhawk ports  == 1.
    
        // @return uint32_t : Number of output streams to build
        //
        virtual uint32_t  getNOutputStreams(); 
 
        //
        // createOutputSRI
        //
        // Called by setupIOMappings when an output mapping is defined. For each output mapping
        // defined, a call to createOutputSRI will be issued with the associated output index.
        // This default SRI and StreamID will be saved to the mapping and pushed down stream via pushSRI.
        // The subclass is responsible for overriding behavior of this method. The index provide matches
        // the stream index number that will be use by the Gnu Radio  Block object
        //
        // @param idx : output stream index number to associate the returned SRI object with
        // @return sri : default SRI object passed down stream over a RedHawk port
        //      
        virtual BULKIO::StreamSRI  createOutputSRI( int32_t oidx, int32_t &in_idx );

        virtual BULKIO::StreamSRI  createOutputSRI( int32_t oidx);

        //
        // adjustOutputRate
        //
        // Called by seOutputStreamSRI method when pushing SRI down stream to adjust the 
        // the xdelta and/or ydelta values accordingly.  The provided method will perform the following:
        //
        //  gr_blocks, gr_sync_block - no modifications are performed
        //  gr_sync_decimator - sri.xdelta * gr_sptr->decimation()
        //  gr_sync_interpolator - sri.xdelta  / gr_sptr->interpolate()
        //
        virtual void  adjustOutputRate(BULKIO::StreamSRI &sri );

        //
        // setOutputSteamSRI
        //
        // Set the SRI context for an output stream from a Gnu Radio Block, when a pushPacket call occurs. Whenever the SRI is established
        // for an output stream it is sent down stream to the next component.
        //  
        virtual void  setOutputStreamSRI( int streamIdx, BULKIO::StreamSRI &in_sri, bool sendSRI=true, bool setStreamID=true ) {
            for (int i = 0; i < (int)io_mapping[streamIdx].size(); i++){
                int o_idx = io_mapping[streamIdx][i];
                _ostreams[o_idx].adjustSRI(in_sri, o_idx, setStreamID );
                if ( sendSRI ) _ostreams[o_idx].pushSRI();
            }
        }

        //
        // setOutputSteamSRI
        //
        // Use the same SRI context for all output streams from a Gnu Radio Block, when a pushPacket call occurs. Whenever the SRI is established
        // for an output stream it is sent down stream to the next component.
        // 
        virtual void  setOutputStreamSRI( BULKIO::StreamSRI &in_sri , bool sendSRI = true, bool setStreamID = true ) {
            OStreamList::iterator ostream=_ostreams.begin();
            for( int o_idx=0;  ostream != _ostreams.end(); o_idx++, ostream++ ) {
                ostream->adjustSRI(in_sri, o_idx, setStreamID );
                if ( sendSRI )  ostream->pushSRI();
            }
        }

    // gr_ostream
    //
    // Provides a mapping of output ports to a Gnu Radio  Block's output stream.  These items
    // are stored in a vector for managing output from the Gnu Radio Block and pushing
    // the data down stream to the next component over the port object.
    //
    // Items in the vector are maintain by setIOMappings, notifySRI and the
    // the service function when "end of stream" happens
    //
    template < typename OUT_PORT_TYPE > struct gr_ostream {
        OUT_PORT_TYPE                      *port;                // handle to Port object
        GNU_RADIO_BLOCK_PTR                grb;                  // shared pointer ot GR_BLOCK
        int                                _idx;                 // output index (loose association)
        std::string                        streamID;             // Stream Id to send down stream
        BULKIO::StreamSRI                  sri;                  // SRI to send down stream
        bool                               _m_tstamp;            // set to true if we are maintaining outgoing time stamp
        BULKIO::PrecisionUTCTime           tstamp;               // time stamp to use for pushPacket calls
        std::vector< typename OUT_PORT_TYPE::NativeType >  _data;    // output buffer used by GR_Block
        bool                               _eos;                        // if EOS was sent
        uint64_t                           _nelems;                     // number of elements in that have been pushed down stream
        int                                _vlen;                      // vector length in items, to allocate output buffer for GR_BLOCK
    
        gr_ostream( OUT_PORT_TYPE *out_port, GNU_RADIO_BLOCK_PTR ingrb, int idx, int mode, std::string &in_sid ) :
            port(out_port), grb(ingrb), _idx(idx), streamID(in_sid), _m_tstamp(false), _data(0), _eos(false), _nelems(0), _vlen(1)
        {
            sri.hversion = 1;
            sri.xstart = 0.0;
            sri.xdelta = 1;
            sri.xunits = BULKIO::UNITS_TIME;
            sri.subsize = 0;
            sri.ystart = 0.0;
            sri.ydelta = 0.0;
            sri.yunits = BULKIO::UNITS_NONE;
            sri.mode = mode;
            sri.streamID = streamID.c_str();
            // RESOLVE sri.blocking=0;   to block or not
    
            tstamp.tcmode = BULKIO::TCM_CPU;
            tstamp.tcstatus = (short)1;
            tstamp.toff = 0.0;
            setTimeStamp();
        };
    
        //
        // translate scalars per element for incoming data
        //    mode == 0 : real, mode == 1 : complex
        static inline int ScalarsPerElement( int mode ) {
            int spe=1;
            if ( mode == 1 ) spe=2;
            return spe;
        };
    
        //
        // translate scalars per element for incoming data
        //    mode == 0 : real, mode == 1 : complex
        static inline int ScalarsPerElement( BULKIO::StreamSRI &sri ) {
            return ScalarsPerElement( sri.mode );
        };
    
        //
        // Return the size of an element (sample) in bytes
        //
        static inline int SizeOfElement(int mode ) {
            return sizeof( typename OUT_PORT_TYPE::NativeType)*ScalarsPerElement( mode);
        };
    
        //
        // Return the size of an element (sample) in bytes
        //
        static inline int SizeOfElement( BULKIO::StreamSRI &sri ) {
            return sizeof( typename OUT_PORT_TYPE::NativeType)*ScalarsPerElement(sri);
        };
    
        //
        // Establish and SRI context for this output stream
        //
        void setSRI( BULKIO::StreamSRI &inSri, int idx ) {
            sri=inSri;
            streamID = sri.streamID;
            // check if history, spe and vlen need to be adjusted
            _check(idx);
        };
    
        //
        // Only adjust stream id and output rate for SRI object
        //
        void adjustSRI( BULKIO::StreamSRI &inSri, int idx, bool setStreamID=true ) {
            if ( setStreamID ) {
                streamID = inSri.streamID;
                sri.streamID = inSri.streamID;
            }
            double ret=inSri.xdelta;
            if ( grb ) ret = ret *grb->relative_rate();
            sri.xdelta = ret;
            _check(idx);
        };
    
        //
        // push our SRI object down stream
        //
        void pushSRI() {
            if ( port ) port->pushSRI( sri );
        };
    
        //
        // push incoming SRI object down stream, do not save this object
        //
        void pushSRI( BULKIO::StreamSRI &inSri ) {
            if ( port ) port->pushSRI( inSri );
        };
    
        //
        // Set our stream ID ...
        //
        void setStreamID( std::string &sid ) {
            streamID=sid;
        };
    
        //
        // Return the number of scalars per element (sample) that we use
        //
        inline int spe() {
            return ScalarsPerElement(sri.mode);
        }
    
        //
        // return the state if EOS was pushed down stream
        //
        inline bool eos () {
            return _eos;
        }
    
        //
        // return the vector length to process data by the GR_BLOCK
        //
        inline int vlen() {
            return _vlen;
        }
    
        inline bool eos ( bool inEos ) {
            _eos=inEos;
            return _eos;
        }
    
        void _check( int idx ) {
            if ( grb ) {
                int nvlen=1;
                try {
                    if ( grb && grb->output_signature() )
                    nvlen = grb->output_signature()->sizeof_stream_item(idx) / (spe() *  sizeof( typename OUT_PORT_TYPE::NativeType));
                    //std::cout << "_CheckVlen IDX: " << _idx << " NEW/OLD VLEN: " << nvlen << "/" << _vlen << " SPE:" << spe() ;
                    if ( nvlen != _vlen && nvlen >= 1 ) _vlen=nvlen;
                } catch(...) {
                    //std::cout << "UNABLE TO SET VLEN, BAD INDEX:" << idx ;
                }
            }
        }
    
        //
        // establish and assocation with a new GR_BLOCK
        //
        void associate( GNU_RADIO_BLOCK_PTR newblock ) {
            grb = newblock;
            _check( _idx );
        }
    
        //
        // return the number of items in the output buffer
        //
        inline uint64_t nitems () {
            uint64_t tmp=nelems();
            if ( _vlen > 0 ) tmp /= _vlen;
            return tmp;
        }
    
        //
        // return the number of elements (samples) in the output buffer
        //
        inline uint64_t  nelems() {
            uint64_t tmp = _data.size();
            if ( spe() > 0 ) tmp /= spe();
            return tmp;
        };
    
    
        //
        // return the number of scalars used for N number of items
        //
        inline uint64_t itemsToScalars( uint64_t N ) {
            return  N*_vlen*spe();
        };
    
        //
        // return the number of output elements sent down stream
        //
        inline uint64_t  oelems() {
            return _nelems;
        };
    
        //
        // return the number of output items sent down stream
        //
        inline uint64_t  oitems() {
            uint64_t tmp = _nelems;
            if ( _vlen > 0 ) tmp /= _vlen;
            return tmp;
        };
    
        //
        // Turn time stamp calculations on or off
        //
        void setAutoAdjustTime( bool onoff ) {
            _m_tstamp = onoff;
        };
    
        //
        // resize the output buffer to N number of items
        //
        void resize( int32_t n_items ) {
            if ( _data.size() != (size_t)(n_items*spe()*_vlen) ) {
                _data.resize( n_items*spe()*_vlen );
            }
        }
    
        typename OUT_PORT_TYPE::NativeType *write_pointer(){
            // push ostream's buffer address onto list of output buffers
            return &(_data[0]);
        }
    
        //
        // sets time stamp value to be time of day
        //
        void setTimeStamp( ) {
            struct timeval tmp_time;
            struct timezone tmp_tz;
            gettimeofday(&tmp_time, &tmp_tz);
            tstamp.twsec = tmp_time.tv_sec;
            tstamp.tfsec = tmp_time.tv_usec / 1e6;
        };
    
        //
        // set time stamp value for the stream to a specific value, turns on
        // stream's monitoring of time stamp
        //
        void setTimeStamp( TimeStamp &inTimeStamp, bool adjust_ts=true ) {
            _m_tstamp = adjust_ts;
            tstamp = inTimeStamp;
        };
    
        void forwardTimeStamp( int32_t noutput_items, TimeStamp &ts ) {
            double twsec = ts.twsec;
            double tfsec = ts.tfsec;
            double sdelta=sri.xdelta;
            sdelta  = sdelta * noutput_items*_vlen;
            double new_time = (twsec+tfsec)+sdelta;
            ts.tfsec = std::modf( new_time, &ts.twsec );
        };
    
        void forwardTimeStamp( int32_t noutput_items ) {
            double twsec = tstamp.twsec;
            double tfsec = tstamp.tfsec;
            double sdelta=sri.xdelta;
            sdelta  = sdelta * noutput_items*_vlen;
            double new_time = (twsec+tfsec)+sdelta;
            tstamp.tfsec = std::modf( new_time, &tstamp.twsec );
        };
    
        //
        // write data to output ports using the provided time stamp and adjust the time
        // accordingly using the xdelta value of the SRI and the number of items
        //
        int  write( int32_t n_items, bool eos, TimeStamp &ts, bool adjust_ts=false ) {
    
            resize( n_items );
    
            if ( port ) port->pushPacket( _data, ts, eos, streamID );
    
            if ( adjust_ts ) forwardTimeStamp( n_items, ts );
    
            _eos = eos;
            _nelems += (n_items*_vlen);
            return n_items;
        };
    
        //
        // write data to the output port using the map object's timestamp
        // if the adjust_ts value equals true. otherwise use time of
        // day for the time stamp
        //
        int  write( int32_t n_items, bool eos, bool adjust_ts ) {
            if ( !adjust_ts ) setTimeStamp();
    
            resize( n_items );
            if ( port ) port->pushPacket( _data, tstamp, eos, streamID );
    
            if ( adjust_ts ) forwardTimeStamp( n_items );
    
            _eos = eos;
            _nelems += (n_items*_vlen);
            return n_items;
        };
    
    
        //
        // write data to the output port using the map object's timestamp and
        // adjust the time stamp if the maps's m_tstamp value == true
        //
        int  write( int32_t n_items, bool eos ) {
            if ( !_m_tstamp ) setTimeStamp();
    
            resize( n_items );
            if ( port ) port->pushPacket( _data, tstamp, eos, streamID );
    
            if ( _m_tstamp ) forwardTimeStamp( n_items );
    
            _eos = eos;
            _nelems += n_items*_vlen;
            return n_items;
        };
    
        // perform clean up on the stream state and map
        void close() {
            _data.clear();
            _vlen=1;
            _eos = false;
            _m_tstamp=false;
        };
    
    };

    template < typename T > class _OStream {
        private:
            _OStream(void) {};
        public:
            typedef typename std::vector< gr_ostream< T > > List;
    };

    typedef  gr_vector_const_void_star                GR_IN_BUFFERS;
    typedef  gr_vector_void_star                      GR_OUT_BUFFERS;
    typedef  gr_vector_int                            GR_BUFFER_LENGTHS;

    template < typename OUT_PORT_TYPE >  int  _generatorServiceFunction( std::vector< gr_ostream< OUT_PORT_TYPE > >  &ostreams );

    typedef _OStream< bulkio::OutOctetPort >::List   OStreamList;


    // cache variables to transferring data to/from a GNU Radio Block
    std::vector<bool>            _input_ready;
    GR_BUFFER_LENGTHS            _ninput_items_required;
    GR_BUFFER_LENGTHS            _ninput_items;
    GR_IN_BUFFERS                _input_items;
    GR_OUT_BUFFERS               _output_items;
    int32_t                      noutput_items;

    // mapping of RH ports to GNU Radio streams
    OStreamList                  _ostreams;
    bool                         sentEOS;


    ENABLE_LOGGING;
    
    protected:

        bool                     _maintainTimeStamp;
        bool                     _throttle;
        TimeMark                 p_start_time;
        TimeMark                 p_end_time;

    public:

        int serviceFunction()
        {
            int retval = NOOP;
            retval = _generatorServiceFunction( _ostreams );

            p_end_time =  boost::posix_time::microsec_clock::local_time();
            if ( retval == NORMAL && _throttle ) {
                TimeDuration  delta = calcThrottle( p_start_time, p_end_time );
                if ( delta.is_not_a_date_time() == false && delta.is_negative() == false ) {
                    LOG_TRACE( glfsr_source_b_base, " SLEEP ...." << delta );
                    boost::this_thread::sleep( delta );
                } else {
                    LOG_TRACE( glfsr_source_b_base, " NO SLEEPING...." );
                }
            }
            p_start_time = p_end_time;
       
            LOG_TRACE( glfsr_source_b_base, " serviceFunction: retval:" << retval);

            return retval;
        };
};
#endif