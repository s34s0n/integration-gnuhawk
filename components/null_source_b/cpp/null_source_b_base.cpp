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

#include "null_source_b_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

null_source_b_base::null_source_b_base(const char *uuid, const char *label) :
    GnuHawkBlock(uuid, label),
    serviceThread(0),
    noutput_items(0),
    _maintainTimeStamp(false),
    _throttle(false)
{
    construct();
}

void null_source_b_base::construct()
{
    Resource_impl::_started = false;
    loadProperties();
    serviceThread = 0;
    sentEOS = false;
    outputPortOrder.resize(0);

    setThrottle(true);

    PortableServer::ObjectId_var oid;
    byte_out = new bulkio::OutOctetPort("byte_out");
    oid = ossie::corba::RootPOA()->activate_object(byte_out);

    registerOutPort(byte_out, byte_out->_this());
    outputPortOrder.push_back("byte_out");

}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void null_source_b_base::initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException)
{
}

void null_source_b_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    boost::mutex::scoped_lock lock(serviceThreadLock);
    if (serviceThread == 0) {
        serviceThread = new ProcessThread<null_source_b_base>(this, 0.1);
        serviceThread->start();
    }
    
    if (!Resource_impl::started()) {
    	Resource_impl::start();
    }
}

void null_source_b_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    // release the child thread (if it exists)
    if (serviceThread != 0) {
      {
        boost::mutex::scoped_lock lock(serviceThreadLock);
        LOG_TRACE( null_source_b_base, "Stopping Service Function" );
        serviceThread->stop();
      }

      if ( !serviceThread->release()) {
         throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
      }

      boost::mutex::scoped_lock lock(serviceThreadLock);
      if ( serviceThread ) {
        delete serviceThread;
      }
    }
    serviceThread = 0;

    if (Resource_impl::started()) {
        Resource_impl::stop();
    }
    
    LOG_TRACE( null_source_b_base, "COMPLETED STOP REQUEST" );
}

CORBA::Object_ptr null_source_b_base::getPort(const char* _id) throw (CORBA::SystemException, CF::PortSupplier::UnknownPort)
{

    std::map<std::string, Port_Provides_base_impl *>::iterator p_in = inPorts.find(std::string(_id));
    if (p_in != inPorts.end()) {
    }

    std::map<std::string, CF::Port_var>::iterator p_out = outPorts_var.find(std::string(_id));
    if (p_out != outPorts_var.end()) {
        return CF::Port::_duplicate(p_out->second);
    }

    throw (CF::PortSupplier::UnknownPort());
}

void null_source_b_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the component running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    // deactivate ports
    releaseInPorts();
    releaseOutPorts();

    delete(byte_out);

    Resource_impl::releaseObject();
}

void null_source_b_base::loadProperties()
{
    addProperty(sizeof_stream_item,
                1,
                "sizeof_stream_item",
                "sizeof_stream_item",
                "readonly",
                "",
                "external",
                "configure");

    addProperty(sampling_freq,
                10000.0,
                "sampling_freq",
                "",
                "readwrite",
                "",
                "external",
                "configure");

}

//
//  Allow for logging 
// 
PREPARE_LOGGING(null_source_b_base);

inline static unsigned int
round_up (unsigned int n, unsigned int multiple)
{
  return ((n + multiple - 1) / multiple) * multiple;
}

inline static unsigned int
round_down (unsigned int n, unsigned int multiple)
{
  return (n / multiple) * multiple;
}

uint32_t null_source_b_base::getNOutputStreams() {
    return 0;
}

void null_source_b_base::setupIOMappings( )
{
    int ninput_streams = 0;
    int noutput_streams = 0;
    std::vector<std::string>::iterator pname;
    std::string sid("");

    if ( !validGRBlock() ) return;
    noutput_streams = gr_sptr->get_max_output_streams();
    gr_io_signature_sptr g_osig = gr_sptr->output_signature();

    LOG_DEBUG( null_source_b_base, "GNUHAWK IO MAPPINGS IN/OUT " << ninput_streams << "/" << noutput_streams );

    //
    // Someone reset the GR Block so we need to clean up old mappings if they exists
    // we need to reset the io signatures and check the vlens
    //
    if ( _ostreams.size() > 0 ) {

        LOG_DEBUG( null_source_b_base, "RESET OUTPUT SIGNATURE SIZE:" << _ostreams.size() );
        OStreamList::iterator ostream;
        for ( int idx=0 ; ostream != _ostreams.end(); idx++, ostream++ ) {
            // need to evaluate new settings...???
            ostream->associate( gr_sptr );
        }

        return;
    }


    //
    // Setup mapping of RH port to GNU RADIO Block input streams
    // For version 1,  we are ignoring the GNU Radio output stream -1 case that allows multiple data 
    // streams over a single connection.  We are mapping a single RH Port to a single GNU Radio stream.
    //
    LOG_TRACE( null_source_b_base, "setupIOMappings OutputPorts: " << outPorts.size() );
    pname = outputPortOrder.begin();
    for( int i=0; pname != outputPortOrder.end(); pname++ ) {

        // grab ports based on their order in the scd.xml file
        RH_UsesPortMap::iterator p_out = outPorts.find(*pname);
        if ( p_out != outPorts.end() ) {
            bulkio::OutOctetPort *port = dynamic_cast< bulkio::OutOctetPort * >(p_out->second);
            int idx = -1;
            BULKIO::StreamSRI sri = createOutputSRI( i, idx );
            if (idx == -1) idx = i;
            if(idx < (int)io_mapping.size()) io_mapping[idx].push_back(i);
            int mode = sri.mode;
            sid = sri.streamID;
            _ostreams.push_back( gr_ostream< bulkio::OutOctetPort > ( port, gr_sptr, i, mode, sid ));
            LOG_DEBUG( null_source_b_base, "ADDING OUTPUT MAP IDX:" << i << " SID:" << sid );
            _ostreams[i].setSRI(sri, i );
            _ostreams[i].pushSRI();
            // increment port counter
            i++;
        }
    }
}

BULKIO::StreamSRI null_source_b_base::createOutputSRI( int32_t oidx ) {
    // for each output stream set the SRI context
    BULKIO::StreamSRI sri = BULKIO::StreamSRI();
    sri.hversion = 1;
    sri.xstart = 0.0;
    sri.xdelta = 1;
    sri.xunits = BULKIO::UNITS_TIME;
    sri.subsize = 0;
    sri.ystart = 0.0;
    sri.ydelta = 0.0;
    sri.yunits = BULKIO::UNITS_NONE;
    sri.mode = 0;
    std::ostringstream t;
    t << naming_service_name.c_str() << "_" << oidx;
    std::string sid = t.str();
    sri.streamID = CORBA::string_dup(sid.c_str());
  
    return sri;
}

BULKIO::StreamSRI null_source_b_base::createOutputSRI( int32_t oidx, int32_t &in_idx)
{
    return createOutputSRI( oidx );
}

void null_source_b_base::adjustOutputRate(BULKIO::StreamSRI &sri )
{
    if ( validGRBlock() ) {
        double ret=sri.xdelta*gr_sptr->relative_rate();
/**
**/
        LOG_TRACE( null_source_b_base, "ADJUSTING SRI.XDELTA FROM/TO: " << sri.xdelta << "/" << ret );
        sri.xdelta = ret;
    }
}

null_source_b_base::TimeDuration null_source_b_base::getTargetDuration()
{
    TimeDuration  t_drate;;
    uint64_t samps=0;
    double   xdelta=1.0;
    double   trate=1.0;

    if ( _ostreams.size() > 0 ) {
        samps= _ostreams[0].nelems();
        xdelta= _ostreams[0].sri.xdelta;
    }

    trate = samps*xdelta;
    uint64_t sec = (uint64_t)trunc(trate);
    uint64_t usec = (uint64_t)((trate-sec)*1e6);
    t_drate = boost::posix_time::seconds(sec) + 
            boost::posix_time::microseconds(usec);
    LOG_TRACE( null_source_b_base, " SEC/USEC " << sec << "/"  << usec << "\n"  <<
              " target_duration " << t_drate );
    return t_drate;
}

null_source_b_base::TimeDuration null_source_b_base::calcThrottle( TimeMark &start_time,
                                             TimeMark &end_time )
{
    TimeDuration delta;
    TimeDuration target_duration = getTargetDuration();

    if ( start_time.is_not_a_date_time() == false ) {
        TimeDuration s_dtime= end_time - start_time;
        delta = target_duration - s_dtime;
        delta /= 4;
        LOG_TRACE( null_source_b_base, " s_time/t_dime " << s_dtime << "/" << target_duration << "\n"  <<
                  " delta " << delta );
    }

    return delta;
}

/**
  DATA GENERATOR TEMPLATE Service Function for GR_BLOCK PATTERN
*/

template < typename OUT_PORT_TYPE > int null_source_b_base::_generatorServiceFunction( typename  std::vector< gr_ostream< OUT_PORT_TYPE > > &ostreams ) 
{

    typedef typename std::vector< gr_ostream< OUT_PORT_TYPE > >  _OStreamList;

    boost::mutex::scoped_lock lock(serviceThreadLock);

    if ( validGRBlock() == false ) {

        // create our processing block, and setup  property notifiers
        createBlock();

        LOG_DEBUG( null_source_b_base, "FINISHED BUILDING  GNU RADIO BLOCK");
    }

    if ( !validGRBlock() || ostreams.size() == 0  ) {
        LOG_WARN( null_source_b_base, "NO OUTPUT STREAMS DEFINED FOR GNU RADIO BLOCK..." );
        return NOOP;
    }

    _ninput_items_required.resize( 0 );
    _ninput_items.resize( 0 );
    _input_items.resize(0);
    _output_items.resize( 0 );

    typename _OStreamList::iterator  ostream;
    noutput_items = gr_pagesize();

    // find transfer length for this block... 
    //   Might want to add per port property and save it off when setupIOMappings is called
    if ( propTable.find("transfer_size") != propTable.end()) {
        CORBA::Any transfer_any;
        CORBA::Long transfer;

        propTable["transfer_size"]->getValue(transfer_any);
        try {
            transfer_any >>= transfer;
            noutput_items = transfer;
        } catch(...) {}
    }

    gr_sptr->forecast(noutput_items, _ninput_items_required);

    LOG_TRACE( null_source_b_base, " FORECAST == " << noutput_items );

    ostream = ostreams.begin();
    for( ; ostream != ostreams.end(); ostream++ ) {
      // push ostream's buffer address onto list of output buffers
      ostream->resize(noutput_items);
      _output_items.push_back((void*)(ostream->write_pointer()) );
    }

    // call the work function
    int numOut=0;
    numOut = gr_sptr->general_work( noutput_items, _ninput_items, _input_items, _output_items);

    bool eos = false;
    // check for stop condition from work method
    if ( numOut == gr_block::WORK_DONE ) {
      numOut = 0;
      eos=true;
    } else {
      sentEOS = false;
    }

    if (numOut != 0 or (eos and !sentEOS)){
        // write out all the data   
        ostream = ostreams.begin();
        for ( ; ostream != ostreams.end(); ostream++ ) {
            LOG_TRACE( null_source_b_base, "PUSHING DATA   NOUT/NITEMS/OITEMS/STREAM_ID " << numOut << 
                      "/" << ostream->nitems()  << "/" << ostream->oitems() << "/" << ostream->streamID );
#ifdef TEST_TIME_STAMP
      LOG_DEBUG( null_source_b_base, "OSTREAM SRI:    xdelta:" << std::setprecision(12) << ostream->sri.xdelta );
      LOG_DEBUG( null_source_b_base, "OSTREAM WRITE:   maint:" << _maintainTimeStamp );
      LOG_DEBUG( null_source_b_base, "                  mode:" <<  ostream->tstamp.tcmode );
      LOG_DEBUG( null_source_b_base, "                status:" <<  ostream->tstamp.tcstatus );
      LOG_DEBUG( null_source_b_base, "                offset:" <<  ostream->tstamp.toff );
      LOG_DEBUG( null_source_b_base, "                 whole:" <<  std::setprecision(10) << ostream->tstamp.twsec );
      LOG_DEBUG( null_source_b_base, "                  frac:" <<  std::setprecision(12) << ostream->tstamp.tfsec );
#endif
            ostream->write( numOut, eos, _maintainTimeStamp );
        }
        if (eos) {
            sentEOS = true;
        }
        // close stream and reset counters  
        ostream = ostreams.begin(); 
        for( ; eos && ostream != ostreams.end(); ostream++ ) {
            ostream->close();
        }

        if (eos) {
            return NOOP;
        }
    }

    return NORMAL;
}


