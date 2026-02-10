#pragma once

#include "flimcompressor.hpp"
#include "flimformat_types.hpp"

// Forward declaration
class encoding_profile;

//  Fletcher checksum -- data size multiple of 2
void fletcher( long &checksum, const std::vector<uint8_t> &data );

//  Computes fletcher, data is big endian
void fletcher( long &checksum, uint16_t data );


//  A completeley encoded flim
class encoded_flim
{
    std::string comment_;

    struct flim_component
    {
        eComponentType type_;
        std::vector<uint8_t> data_;

        void fletcher( long &checksum ) const
        {
            ::fletcher( checksum, data_  );
        }
    };

    std::vector<flim_component> components_;

public:
    encoded_flim( const std::string &comment ) : comment_{ comment }
    {
            //  Comment is exactly 1022 characters ans starts with 'FLIM\n'
        if (comment_.size()<1022)
            comment_.resize( 1022, 0x00);
        else
            comment_ = comment_.substr(0, 1022);

        comment_[0] = 'F';
        comment_[1] = 'L';
        comment_[2] = 'I';
        comment_[3] = 'M';
        comment_[4] = '\n';
    }

    void add_component( eComponentType type, const std::vector<uint8_t> data )
    {
        components_.emplace_back( flim_component{ type, data } );
    }

        //  Adds the flim info component
    void add( const flim_info &fi )
    {
        std::vector<uint8_t> data;
        fi.serialize( data );
        add_component( component_info, data );
    }

        //  Adds all the frames and generate the movie and toc component
    void add( const std::vector<flimcompressor::frame> &frames )
    {
        std::vector<uint8_t> movie_data;
        std::vector<uint8_t> toc_data;

        auto om = std::back_inserter(movie_data);
        auto ot = std::back_inserter(toc_data);

        size_t prev_size = 0;

        for (auto &f:frames)
        {
            write2( om, f.ticks );

            auto audio = f.audio;
            if (audio.size()==0)
                write2( om, 2 );           //  2 means an empty sound block
            else
            {
                write2( om, f.ticks*370+8 );           //  size of sound + header + size itself
                write2( om, 0 );                       //  ffMode
                write4( om, 65536 );                   //  rate
                write( om, f.audio );
            }

            write2( om, f.video.size()+2 );
            write( om, f.video );

            //  TOC entry for current frame (size in bytes of the frame)
            write2( ot, movie_data.size()-prev_size );
            prev_size = movie_data.size();
        }

        add_component( component_movie, movie_data );
        add_component( component_toc, toc_data );
    }

    void add_framebuffer( eComponentType type, const framebuffer &fb )
    {
        std::vector<uint8_t> data;
        auto bi = std::back_inserter(data);
        write2( bi, 0x00 );         //  Framebuffer
        write2( bi, fb.W() ); 
        write2( bi, fb.H() ); 
        auto img_data = fb.raw_values_natural<uint8_t>();
        data.insert( std::end(data), std::begin(img_data), std::end(img_data) );
        add_component( type, data );
    }

    void add_poster( const framebuffer &fb )
    {
        std::vector<uint8_t> data = fb.raw_values_natural<uint8_t>();
        add_component( component_poster, data );
    }

    void add_initial( const framebuffer &fb )
    {
        add_framebuffer( component_initial, fb );
    }

    std::vector<uint8_t> header() const
    {
        std::vector<uint8_t> header;
        auto o = std::back_inserter(header);

        write2( o, 0x1 );                      //  Version
        write2( o, components_.size() );          //  Entry count

        size_t offset = 0;

        for (auto &c:components_)
        {
            write2( o, c.type_ );
            write4( o, offset );
            write4( o, c.data_.size() );
            offset += c.data_.size();
        }

        return header;
    }

    uint16_t fletcher() const
    {
        long checksum = 0;
        ::fletcher( checksum, header() );

        printf( "HEADER: %ld\n", checksum );
        for (auto &c:components_)
        {
            c.fletcher( checksum );
            printf( "-> : %ld\n", checksum );
        }

        return checksum;
    }

    void fwrite( uint16_t v, FILE *f ) const
    {
        uint8_t b = v/256;
        ::fwrite( &b, 1, 1, f );
        b = v%256;
        ::fwrite( &b, 1, 1, f );
    }

    void fwrite( const std::vector<uint8_t> &v, FILE *f ) const
    {
        ::fwrite( v.data(), v.size(), 1, f );
    }

    void fwrite( FILE *f ) const
    {
        ::fwrite( comment_.c_str(), 1022, 1, f );
        fwrite( fletcher(), f );
        fwrite( header(), f );
        for (auto &c:components_)
            fwrite( c.data_, f );
    }
};
