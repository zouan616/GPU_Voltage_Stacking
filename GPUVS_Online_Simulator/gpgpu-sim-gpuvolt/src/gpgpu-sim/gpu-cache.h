// Copyright (c) 2009-2011, Tor M. Aamodt, Tayler Hetherington
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GPU_CACHE_H
#define GPU_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include "gpu-misc.h"
#include "mem_fetch.h"
#include "../abstract_hardware_model.h"
#include "../tr1_hash_map.h"

#include "addrdec.h"

enum cache_block_state {
    INVALID,
    RESERVED,
    VALID,
    MODIFIED
};

enum cache_request_status {
    HIT,
    HIT_RESERVED,
    MISS,
    RESERVATION_FAIL
};

enum cache_event {
    WRITE_BACK_REQUEST_SENT,
    READ_REQUEST_SENT,
    WRITE_REQUEST_SENT
};

struct cache_block_t {
    cache_block_t()
    {
        m_tag=0;
        m_block_addr=0;
        m_alloc_time=0;
        m_fill_time=0;
        m_last_access_time=0;
        m_status=INVALID;
    }
    void allocate( new_addr_type tag, new_addr_type block_addr, unsigned time )
    {
        m_tag=tag;
        m_block_addr=block_addr;
        m_alloc_time=time;
        m_last_access_time=time;
        m_fill_time=0;
        m_status=RESERVED;
    }
    void fill( unsigned time )
    {
        assert( m_status == RESERVED );
        m_status=VALID;
        m_fill_time=time;
    }

    new_addr_type    m_tag;
    new_addr_type    m_block_addr;
    unsigned         m_alloc_time;
    unsigned         m_last_access_time;
    unsigned         m_fill_time;
    cache_block_state    m_status;
};

enum replacement_policy_t {
    LRU,
    FIFO
};

enum write_policy_t {
    READ_ONLY,
    WRITE_BACK,
    WRITE_THROUGH,
    WRITE_EVICT,
    LOCAL_WB_GLOBAL_WT
};

enum allocation_policy_t {
    ON_MISS,
    ON_FILL
};


enum write_allocate_policy_t {
	NO_WRITE_ALLOCATE,
	WRITE_ALLOCATE
};

enum mshr_config_t {
    TEX_FIFO,
    ASSOC // normal cache 
};


class cache_config {
public:
    cache_config() 
    { 
        m_valid = false; 
        m_disabled = false;
        m_config_string = NULL; // set by option parser
    }
    void init()
    {
        assert( m_config_string );
        char rp, wp, ap, mshr_type, wap;

        int ntok = sscanf(m_config_string,"%u:%u:%u,%c:%c:%c:%c,%c:%u:%u,%u:%u",
                          &m_nset, &m_line_sz, &m_assoc, &rp, &wp, &ap, &wap,
                          &mshr_type, &m_mshr_entries,&m_mshr_max_merge,
                          &m_miss_queue_size,&m_result_fifo_entries);

        if ( ntok < 10 ) {
            if ( !strcmp(m_config_string,"none") ) {
                m_disabled = true;
                return;
            }
            exit_parse_error();
        }
        switch (rp) {
        case 'L': m_replacement_policy = LRU; break;
        case 'F': m_replacement_policy = FIFO; break;
        default: exit_parse_error();
        }
        switch (wp) {
        case 'R': m_write_policy = READ_ONLY; break;
        case 'B': m_write_policy = WRITE_BACK; break;
        case 'T': m_write_policy = WRITE_THROUGH; break;
        case 'E': m_write_policy = WRITE_EVICT; break;
        case 'L': m_write_policy = LOCAL_WB_GLOBAL_WT; break;
        default: exit_parse_error();
        }
        switch (ap) {
        case 'm': m_alloc_policy = ON_MISS; break;
        case 'f': m_alloc_policy = ON_FILL; break;
        default: exit_parse_error();
        }
        switch (mshr_type) {
        case 'F': m_mshr_type = TEX_FIFO; assert(ntok==12); break;
        case 'A': m_mshr_type = ASSOC; break;
        default: exit_parse_error();
        }
        m_line_sz_log2 = LOGB2(m_line_sz);
        m_nset_log2 = LOGB2(m_nset);
        m_valid = true;

        switch(wap){
        case 'W': m_write_alloc_policy = WRITE_ALLOCATE; break;
        case 'N': m_write_alloc_policy = NO_WRITE_ALLOCATE; break;
        default: exit_parse_error();
        }

        // detect invalid configuration 
        if (m_alloc_policy == ON_FILL and m_write_policy == WRITE_BACK) {
            // A writeback cache with allocate-on-fill policy will inevitably lead to deadlock:  
            // The deadlock happens when an incoming cache-fill evicts a dirty
            // line, generating a writeback request.  If the memory subsystem
            // is congested, the interconnection network may not have
            // sufficient buffer for the writeback request.  This stalls the
            // incoming cache-fill.  The stall may propagate through the memory
            // subsystem back to the output port of the same core, creating a
            // deadlock where the wrtieback request and the incoming cache-fill
            // are stalling each other.  
            assert(0 && "Invalid cache configuration: Writeback cache cannot allocate new line on fill. "); 
        }
    }
    bool disabled() const { return m_disabled;}
    unsigned get_line_sz() const
    {
        assert( m_valid );
        return m_line_sz;
    }
    unsigned get_num_lines() const
    {
        assert( m_valid );
        return m_nset * m_assoc;
    }

    void print( FILE *fp ) const
    {
        fprintf( fp, "Size = %d B (%d Set x %d-way x %d byte line)\n", 
                 m_line_sz * m_nset * m_assoc,
                 m_nset, m_assoc, m_line_sz );
    }

    virtual unsigned set_index( new_addr_type addr ) const
    {
        return(addr >> m_line_sz_log2) & (m_nset-1);
    }

    new_addr_type tag( new_addr_type addr ) const
    {
    	// For generality, the tag includes both index and tag. This allows for more complex set index
    	// calculations that can result in different indexes mapping to the same set, thus the full
    	// tag + index is required to check for hit/miss. Tag is now identical to the block address.

        //return addr >> (m_line_sz_log2+m_nset_log2);
    	return addr & ~(m_line_sz-1);
    }
    new_addr_type block_addr( new_addr_type addr ) const
    {
        return addr & ~(m_line_sz-1);
    }

    char *m_config_string;

protected:
    void exit_parse_error()
    {
        printf("GPGPU-Sim uArch: cache configuration parsing error (%s)\n", m_config_string );
        abort();
    }

    bool m_valid;
    bool m_disabled;
    unsigned m_line_sz;
    unsigned m_line_sz_log2;
    unsigned m_nset;
    unsigned m_nset_log2;
    unsigned m_assoc;

    enum replacement_policy_t m_replacement_policy; // 'L' = LRU, 'F' = FIFO
    enum write_policy_t m_write_policy;             // 'T' = write through, 'B' = write back, 'R' = read only
    enum allocation_policy_t m_alloc_policy;        // 'm' = allocate on miss, 'f' = allocate on fill
    enum mshr_config_t m_mshr_type;

    write_allocate_policy_t m_write_alloc_policy;	// 'W' = Write allocate, 'N' = No write allocate

    union {
        unsigned m_mshr_entries;
        unsigned m_fragment_fifo_entries;
    };
    union {
        unsigned m_mshr_max_merge;
        unsigned m_request_fifo_entries;
    };
    union {
        unsigned m_miss_queue_size;
        unsigned m_rob_entries;
    };
    unsigned m_result_fifo_entries;

    friend class tag_array;
    friend class baseline_cache;
    friend class read_only_cache;
    friend class tex_cache;
    friend class data_cache;
    friend class l1_cache;
    friend class l2_cache;
};


class l2_cache_config : public cache_config {
public:
	l2_cache_config() : cache_config(){}
	void init(linear_to_raw_address_translation *address_mapping);
	virtual unsigned set_index(new_addr_type addr) const;

private:
	linear_to_raw_address_translation *m_address_mapping;
};

class tag_array {
public:
    // Use this constructor
    tag_array( const cache_config &config, int core_id, int type_id );
    ~tag_array();

    enum cache_request_status probe( new_addr_type addr, unsigned &idx ) const;
    enum cache_request_status access( new_addr_type addr, unsigned time, unsigned &idx );
    enum cache_request_status access( new_addr_type addr, unsigned time, unsigned &idx, bool &wb, cache_block_t &evicted );

    void fill( new_addr_type addr, unsigned time );
    void fill( unsigned idx, unsigned time );

    unsigned size() const { return m_config.get_num_lines();}
    cache_block_t &get_block(unsigned idx) { return m_lines[idx];}

    void flush(); // flash invalidate all entries
    void new_window();

    void print( FILE *stream, unsigned &total_access, unsigned &total_misses ) const;
    float windowed_miss_rate( ) const;
    void get_stats(unsigned &total_access, unsigned &total_misses) const;
	// Jingwen: returnt the core id where this cache belongs to 
	int  get_core_id() {return m_core_id;}

protected:
    // This constructor is intended for use only from derived classes that wish to
    // avoid unnecessary memory allocation that takes place in the
    // other tag_array constructor
    tag_array( const cache_config &config,
               int core_id,
               int type_id,
               cache_block_t* new_lines );
    void init( int core_id, int type_id );

protected:

    const cache_config &m_config;

    cache_block_t *m_lines; /* nbanks x nset x assoc lines in total */

    unsigned m_access;
    unsigned m_miss;
    unsigned m_pending_hit; // number of cache miss that hit a line that is allocated but not filled

    // performance counters for calculating the amount of misses within a time window
    unsigned m_prev_snapshot_access;
    unsigned m_prev_snapshot_miss;
    unsigned m_prev_snapshot_pending_hit;

    int m_core_id; // which shader core is using this
    int m_type_id; // what kind of cache is this (normal, texture, constant)
};


class mshr_table {
public:
    mshr_table( unsigned num_entries, unsigned max_merged )
    : m_num_entries(num_entries),
    m_max_merged(max_merged)
#if (tr1_hash_map_ismap == 0)
    ,m_data(2*num_entries)
#endif
    {
    }

    /// Checks if there is a pending request to the lower memory level already
    bool probe( new_addr_type block_addr ) const;
    /// Checks if there is space for tracking a new memory access
    bool full( new_addr_type block_addr ) const;
    /// Add or merge this access
    void add( new_addr_type block_addr, mem_fetch *mf );
    /// Returns true if cannot accept new fill responses
    bool busy() const {return false;}
    /// Accept a new cache fill response: mark entry ready for processing
    void mark_ready( new_addr_type block_addr, bool &has_atomic );
    /// Returns true if ready accesses exist
    bool access_ready() const {return !m_current_response.empty();}
    /// Returns next ready access
    mem_fetch *next_access();
    void display( FILE *fp ) const;

private:

    // finite sized, fully associative table, with a finite maximum number of merged requests
    const unsigned m_num_entries;
    const unsigned m_max_merged;

    struct mshr_entry {
        std::list<mem_fetch*> m_list;
        bool m_has_atomic; 
        mshr_entry() : m_has_atomic(false) { }
    }; 
    typedef tr1_hash_map<new_addr_type,mshr_entry> table;
    table m_data;

    // it may take several cycles to process the merged requests
    bool m_current_response_ready;
    std::list<new_addr_type> m_current_response;
};


/***************************************************************** Caches *****************************************************************/

class cache_t {
public:
    virtual ~cache_t() {}
    virtual enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events ) =  0;
};

bool was_write_sent( const std::list<cache_event> &events );
bool was_read_sent( const std::list<cache_event> &events );

/// Baseline cache
/// Implements common functions for read_only_cache and data_cache
/// Each subclass implements its own 'access' function
class baseline_cache : public cache_t {
public:
    baseline_cache( const char *name, const cache_config &config, int core_id, int type_id, mem_fetch_interface *memport,
                     enum mem_fetch_status status )
    : m_config(config), m_tag_array(new tag_array(config,core_id,type_id)), m_mshrs(config.m_mshr_entries,config.m_mshr_max_merge)
    {
        init( name, config, memport, status );
    }

    void init( const char *name,
               const cache_config &config,
               mem_fetch_interface *memport,
               enum mem_fetch_status status )
    {
        m_name = name;
        assert(config.m_mshr_type == ASSOC);
        m_memport=memport;
        m_miss_queue_status = status;
        m_read_access=0;
        m_write_access=0;
        m_read_miss=0;
        m_write_miss=0;
        n_simt_to_mem=0;
    }

    virtual ~baseline_cache()
    {
        delete m_tag_array;
    }

    virtual enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events ) =  0;
    /// Sends next request to lower level of memory
    void cycle();
    /// Interface for response from lower memory level (model bandwidth restictions in caller)
    void fill( mem_fetch *mf, unsigned time );
    /// Checks if mf is waiting to be filled by lower memory level
    bool waiting_for_fill( mem_fetch *mf );
    /// Are any (accepted) accesses that had to wait for memory now ready? (does not include accesses that "HIT")
    bool access_ready() const {return m_mshrs.access_ready();}
    /// Pop next ready access (does not include accesses that "HIT")
    mem_fetch *next_access(){return m_mshrs.next_access();}
    // flash invalidate all entries in cache
    void flush(){m_tag_array->flush();}
    void print(FILE *fp, unsigned &accesses, unsigned &misses) const;
    void display_state( FILE *fp ) const;

    virtual void get_data_stats(unsigned &read_access, unsigned &read_misses,unsigned &write_access,  unsigned &write_misses) const {
    	read_access = m_read_access;
    	write_access = m_write_access;
    	read_misses = m_read_miss;
    	write_misses = m_write_miss;
    }

    void get_stats(unsigned &accesses, unsigned &misses) const {
    	m_tag_array->get_stats(accesses, misses);
    }

    void set_icnt_power_stats(unsigned &simt_to_mem) const{
    	simt_to_mem = n_simt_to_mem;
    }



protected:
    // Constructor that can be used by derived classes with custom tag arrays
    baseline_cache( const char *name,
                    const cache_config &config,
                    int core_id,
                    int type_id,
                    mem_fetch_interface *memport,
                    enum mem_fetch_status status,
                    tag_array* new_tag_array )
    : m_config(config),
      m_tag_array( new_tag_array ),
      m_mshrs(config.m_mshr_entries,config.m_mshr_max_merge)
    {
        init( name, config, memport, status );
    }

protected:
    std::string m_name;
    const cache_config &m_config;
    tag_array*  m_tag_array;
    mshr_table m_mshrs;
    std::list<mem_fetch*> m_miss_queue;
    enum mem_fetch_status m_miss_queue_status;
    mem_fetch_interface *m_memport;

    struct extra_mf_fields {
        extra_mf_fields()  { m_valid = false;}
        extra_mf_fields( new_addr_type a, unsigned i, unsigned d ) 
        {
            m_valid = true;
            m_block_addr = a;
            m_cache_index = i;
            m_data_size = d;
        }
        bool m_valid;
        new_addr_type m_block_addr;
        unsigned m_cache_index;
        unsigned m_data_size;
    };

    typedef std::map<mem_fetch*,extra_mf_fields> extra_mf_fields_lookup;

    extra_mf_fields_lookup m_extra_mf_fields;

    /// Checks whether this request can be handled on this cycle. num_miss equals max # of misses to be handled on this cycle
    bool miss_queue_full(unsigned num_miss){
    	  return ( (m_miss_queue.size()+num_miss) >= m_config.m_miss_queue_size );
    }
    /// Read miss handler without writeback
    void send_read_request(new_addr_type addr, new_addr_type block_addr, unsigned cache_index, mem_fetch *mf,
    		unsigned time, bool &do_miss, std::list<cache_event> &events, bool read_only, bool wa);
    /// Read miss handler. Check MSHR hit or MSHR available
    void send_read_request(new_addr_type addr, new_addr_type block_addr, unsigned cache_index, mem_fetch *mf,
    		unsigned time, bool &do_miss, bool &wb, cache_block_t &evicted, std::list<cache_event> &events, bool read_only, bool wa);

    // Power stats
    unsigned m_read_access;
    unsigned m_write_access;
    unsigned m_read_miss;
    unsigned m_write_miss;

    unsigned n_simt_to_mem; // Interconnect power stats
};

/// Read only cache
class read_only_cache : public baseline_cache {
public:
    read_only_cache( const char *name, const cache_config &config, int core_id, int type_id, mem_fetch_interface *memport, enum mem_fetch_status status )
    : baseline_cache(name,config,core_id,type_id,memport,status){}

    /// Access cache for read_only_cache: returns RESERVATION_FAIL if request could not be accepted (for any reason)
    virtual enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events );

    virtual ~read_only_cache(){}

protected:
    read_only_cache( const char *name, const cache_config &config, int core_id, int type_id, mem_fetch_interface *memport, enum mem_fetch_status status, tag_array* new_tag_array )
    : baseline_cache(name,config,core_id,type_id,memport,status, new_tag_array){}
};

/// Data cache - Implements common functions for L1 and L2 data cache
class data_cache : public baseline_cache {
public:
    data_cache( const char *name, const cache_config &config,
    			int core_id, int type_id, mem_fetch_interface *memport,
                mem_fetch_allocator *mfcreator, enum mem_fetch_status status )
    			: baseline_cache(name,config,core_id,type_id,memport,status)
    {
        init( mfcreator );
    }

    virtual ~data_cache() {}

    void init( mem_fetch_allocator *mfcreator )
    {
        m_memfetch_creator=mfcreator;

        // Set read hit function
        m_rd_hit = &data_cache::rd_hit_base;

        // Set read miss function
        m_rd_miss = &data_cache::rd_miss_base;

        // Set write hit function
        switch(m_config.m_write_policy){
        case READ_ONLY: assert(0 && "Error: Writable Data_cache set as READ_ONLY\n"); break; // READ_ONLY is now a separate cache class, config is deprecated
        case WRITE_BACK: m_wr_hit = &data_cache::wr_hit_wb; break;
        case WRITE_THROUGH: m_wr_hit = &data_cache::wr_hit_wt; break;
        case WRITE_EVICT: m_wr_hit = &data_cache::wr_hit_we; break;
        case LOCAL_WB_GLOBAL_WT: m_wr_hit = &data_cache::wr_hit_global_we_local_wb; break;
        default: assert(0 && "Error: Must set valid cache write policy\n"); break; // Need to set a write hit function
        }

        // Set write miss function
        switch(m_config.m_write_alloc_policy){
        case WRITE_ALLOCATE: m_wr_miss = &data_cache::wr_miss_wa; break;
        case NO_WRITE_ALLOCATE: m_wr_miss = &data_cache::wr_miss_no_wa; break;
        default: assert(0 && "Error: Must set valid cache write miss policy\n"); break; // Need to set a write miss function
        }
    }

protected:
    data_cache( const char *name,
                const cache_config &config,
    			int core_id,
                int type_id,
                mem_fetch_interface *memport,
                mem_fetch_allocator *mfcreator,
                enum mem_fetch_status status,
                tag_array* new_tag_array )
    : baseline_cache(name, config, core_id, type_id, memport,status, new_tag_array)
    {
        init( mfcreator );
    }

protected:
    mem_fetch_allocator *m_memfetch_creator;

    // Functions for data cache access
    /// Sends write request to lower level memory (write or writeback)
    void send_write_request(mem_fetch *mf, cache_event request, unsigned time, std::list<cache_event> &events);


    // Member Function pointers - Set by configuration options to the functions below each grouping
    /******* Write-hit configs *******/
    enum cache_request_status (data_cache::*m_wr_hit)(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);
    /// Marks block as MODIFIED and updates block LRU
    enum cache_request_status wr_hit_wb(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // write-back
    enum cache_request_status wr_hit_wt(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // write-through
    /// Marks block as INVALID and sends write request to lower level memory
    enum cache_request_status wr_hit_we(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // write-evict
    enum cache_request_status wr_hit_global_we_local_wb(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // global write-evict, local write-back


    /******* Write-miss configs *******/
    enum cache_request_status (data_cache::*m_wr_miss)(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);
    /// Sends read request, and possible write-back request, to lower level memory for a write miss with write-allocate
    enum cache_request_status wr_miss_wa(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // write-allocate
    enum cache_request_status wr_miss_no_wa(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status); // no write-allocate

    // Currently no separate functions for reads
    /******* Read-hit configs *******/
    enum cache_request_status (data_cache::*m_rd_hit)(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);
    enum cache_request_status rd_hit_base(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);

    /******* Read-miss configs *******/
    enum cache_request_status (data_cache::*m_rd_miss)(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);
    enum cache_request_status rd_miss_base(new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time, std::list<cache_event> &events, enum cache_request_status status);

};

/// This is meant to model the first level data cache in Fermi.
/// It is write-evict (global) or write-back (local) at the granularity of individual blocks
/// (the policy used in fermi according to the CUDA manual)
class l1_cache : public data_cache {
public:
	l1_cache(const char *name, const cache_config &config,
			int core_id, int type_id, mem_fetch_interface *memport,
            mem_fetch_allocator *mfcreator, enum mem_fetch_status status )
			: data_cache(name,config,core_id,type_id,memport,mfcreator,status){}

    virtual ~l1_cache(){}

	virtual enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events );

	// Jingwen: print miss queue
	void print_miss_queue( FILE * pf) {
		fprintf(pf, "  [miss queue] %d/%d\n", m_miss_queue.size(), m_config.m_miss_queue_size);
		std::list<mem_fetch*>::iterator  iter, iter_begin, iter_end;	
		iter_begin = m_miss_queue.begin();
		iter_end   = m_miss_queue.end();
		unsigned item_count = 1;
		for ( iter = iter_begin; iter != iter_end; iter++) {
			mem_fetch * mf = *iter;
			fprintf(pf, "    [item %d] W-%d PC-%X Addr-%X\n", item_count, mf->get_wid(), mf->get_pc(), mf->get_addr());
			item_count++;
		}
	}

protected:
	l1_cache( const char *name,
              const cache_config &config,
			  int core_id,
              int type_id,
              mem_fetch_interface *memport,
              mem_fetch_allocator *mfcreator,
              enum mem_fetch_status status,
              tag_array* new_tag_array )
	: data_cache(name,config,core_id,type_id,memport,mfcreator,status, new_tag_array){}

};

/// Models second level shared cache with global write-back and write-allocate policies
class l2_cache : public data_cache {
public:
	l2_cache(const char *name, const cache_config &config,
			int core_id, int type_id, mem_fetch_interface *memport,
            mem_fetch_allocator *mfcreator, enum mem_fetch_status status )
			: data_cache(name,config,core_id,type_id,memport,mfcreator,status){}

    virtual ~l2_cache() {}

	virtual enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events );

};

/********************************************************************************************************************************************************/

// See the following paper to understand this cache model:
// 
// Igehy, et al., Prefetching in a Texture Cache Architecture, 
// Proceedings of the 1998 Eurographics/SIGGRAPH Workshop on Graphics Hardware
// http://www-graphics.stanford.edu/papers/texture_prefetch/
class tex_cache : public cache_t {
public:
    tex_cache( const char *name, const cache_config &config, int core_id, int type_id, mem_fetch_interface *memport,
               enum mem_fetch_status request_status, 
               enum mem_fetch_status rob_status )
    : m_config(config), 
    m_tags(config,core_id,type_id), 
    m_fragment_fifo(config.m_fragment_fifo_entries), 
    m_request_fifo(config.m_request_fifo_entries),
    m_rob(config.m_rob_entries),
    m_result_fifo(config.m_result_fifo_entries)
    {
        m_name = name;
        assert(config.m_mshr_type == TEX_FIFO);
        assert(config.m_write_policy == READ_ONLY);
        assert(config.m_alloc_policy == ON_MISS);
        m_memport=memport;
        m_cache = new data_block[ config.get_num_lines() ];
        m_request_queue_status = request_status;
        m_rob_status = rob_status;
        n_simt_to_mem=0;
    }

    /// Access function for tex_cache
    /// return values: RESERVATION_FAIL if request could not be accepted
    /// otherwise returns HIT_RESERVED or MISS; NOTE: *never* returns HIT
    /// since unlike a normal CPU cache, a "HIT" in texture cache does not
    /// mean the data is ready (still need to get through fragment fifo)
    enum cache_request_status access( new_addr_type addr, mem_fetch *mf, unsigned time, std::list<cache_event> &events );
    void cycle();
    /// Place returning cache block into reorder buffer
    void fill( mem_fetch *mf, unsigned time );
    /// Are any (accepted) accesses that had to wait for memory now ready? (does not include accesses that "HIT")
    bool access_ready() const{return !m_result_fifo.empty();}
    /// Pop next ready access (includes both accesses that "HIT" and those that "MISS")
    mem_fetch *next_access(){return m_result_fifo.pop();}
    void display_state( FILE *fp ) const;

    void get_stats(unsigned &accesses, unsigned &misses) const{
    	m_tags.get_stats(accesses, misses);
    }

    void set_icnt_power_stats(unsigned &simt_to_mem) const{
    	simt_to_mem = n_simt_to_mem;
    }

    private:
    std::string m_name;
    const cache_config &m_config;

    struct fragment_entry {
        fragment_entry() {}
        fragment_entry( mem_fetch *mf, unsigned idx, bool m, unsigned d )
        {
            m_request=mf;
            m_cache_index=idx;
            m_miss=m;
            m_data_size=d;
        }
        mem_fetch *m_request;     // request information
        unsigned   m_cache_index; // where to look for data
        bool       m_miss;        // true if sent memory request
        unsigned   m_data_size;
    };

    struct rob_entry {
        rob_entry() { m_ready = false; m_time=0; m_request=NULL;}
        rob_entry( unsigned i, mem_fetch *mf, new_addr_type a ) 
        { 
            m_ready=false; 
            m_index=i;
            m_time=0;
            m_request=mf; 
            m_block_addr=a;
        }
        bool m_ready;
        unsigned m_time; // which cycle did this entry become ready?
        unsigned m_index; // where in cache should block be placed?
        mem_fetch *m_request;
        new_addr_type m_block_addr;
    };

    struct data_block {
        data_block() { m_valid = false;}
        bool m_valid;
        new_addr_type m_block_addr;
    };

    // TODO: replace fifo_pipeline with this?
    template<class T> class fifo {
    public:
        fifo( unsigned size ) 
        { 
            m_size=size; 
            m_num=0; 
            m_head=0; 
            m_tail=0; 
            m_data = new T[size];
        }
        bool full() const { return m_num == m_size;}
        bool empty() const { return m_num == 0;}
        unsigned size() const { return m_num;}
        unsigned capacity() const { return m_size;}
        unsigned push( const T &e ) 
        { 
            assert(!full()); 
            m_data[m_head] = e; 
            unsigned result = m_head;
            inc_head(); 
            return result;
        }
        T pop() 
        { 
            assert(!empty()); 
            T result = m_data[m_tail];
            inc_tail();
            return result;
        }
        const T &peek( unsigned index ) const 
        { 
            assert( index < m_size );
            return m_data[index]; 
        }
        T &peek( unsigned index ) 
        { 
            assert( index < m_size );
            return m_data[index]; 
        }
        T &peek() const
        { 
            return m_data[m_tail]; 
        }
        unsigned next_pop_index() const 
        {
            return m_tail;
        }
    private:
        void inc_head() { m_head = (m_head+1)%m_size; m_num++;}
        void inc_tail() { assert(m_num>0); m_tail = (m_tail+1)%m_size; m_num--;}

        unsigned   m_head; // next entry goes here
        unsigned   m_tail; // oldest entry found here
        unsigned   m_num;  // how many in fifo?
        unsigned   m_size; // maximum number of entries in fifo
        T         *m_data;
    };

    tag_array               m_tags;
    fifo<fragment_entry>    m_fragment_fifo;
    fifo<mem_fetch*>        m_request_fifo;
    fifo<rob_entry>         m_rob;
    data_block             *m_cache;
    fifo<mem_fetch*>        m_result_fifo; // next completed texture fetch

    mem_fetch_interface    *m_memport;
    enum mem_fetch_status   m_request_queue_status;
    enum mem_fetch_status   m_rob_status;

    struct extra_mf_fields {
        extra_mf_fields()  { m_valid = false;}
        extra_mf_fields( unsigned i ) 
        {
            m_valid = true;
            m_rob_index = i;
        }
        bool m_valid;
        unsigned m_rob_index;
    };

    typedef std::map<mem_fetch*,extra_mf_fields> extra_mf_fields_lookup;

    extra_mf_fields_lookup m_extra_mf_fields;

    // Interconnect power stats
    unsigned n_simt_to_mem;
};

#endif
