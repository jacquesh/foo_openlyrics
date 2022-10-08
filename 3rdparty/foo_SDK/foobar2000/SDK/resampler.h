#pragma once

#ifdef FOOBAR2000_HAVE_DSP

//! A resampler DSP entry. \n
//! It is STRICTLY REQUIRED that the output is: \n
//! (A) In the requested sample rate (specified when creating the preset), \n
//! (B) .. or untouched if the conversion cannot be performed / there's no conversion to be performed (input rate == output rate). \n
//! Not every resampler supports every possible sample rate conversion ratio. Bundled PPHS resampler (always installed since foobar2000 v1.6) does accept every possible conversion.
class NOVTABLE resampler_entry : public dsp_entry
{
public:
	virtual bool is_conversion_supported(unsigned p_srate_from,unsigned p_srate_to) = 0;
	virtual bool create_preset(dsp_preset & p_out,unsigned p_target_srate,float p_qualityscale) = 0;//p_qualityscale is 0...1
	virtual float get_priority() = 0;//value is 0...1, where high-quality (SSRC etc) has 1

	static bool g_get_interface(service_ptr_t<resampler_entry> & p_out,unsigned p_srate_from,unsigned p_srate_to);
	static bool g_create(service_ptr_t<dsp> & p_out,unsigned p_srate_from,unsigned p_srate_to,float p_qualityscale);
	static bool g_create_preset(dsp_preset & p_out,unsigned p_srate_from,unsigned p_srate_to,float p_qualityscale);

	FB2K_MAKE_SERVICE_INTERFACE(resampler_entry,dsp_entry);
};

template<typename T>
class resampler_entry_impl_t : public dsp_entry_impl_t<T,resampler_entry>
{
public:
	bool is_conversion_supported(unsigned p_srate_from,unsigned p_srate_to) {return T::g_is_conversion_supported(p_srate_from,p_srate_to);}
	bool create_preset(dsp_preset & p_out,unsigned p_target_srate,float p_qualityscale) {return T::g_create_preset(p_out,p_target_srate,p_qualityscale);}
	float get_priority() {return T::g_get_priority();}
};

template<typename T>
class resampler_factory_t : public service_factory_single_t<resampler_entry_impl_t<T> > {};

#ifdef FOOBAR2000_DESKTOP

//! \since 1.4
//! Supersedes resampler_entry::get_priority, allows the user to specify which resampler should be preferred when a component asks for one.
class resampler_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(resampler_manager);
public:
	//! Locate the preferred resampler that is capable of performing conversion from the source to destination rate. \n
	//! If input sample rate is not known in advance or may change in mid-conversion, it's recommended to use make_chain() instead to full obey user settings.
	virtual resampler_entry::ptr get_resampler( unsigned rateFrom, unsigned rateTo ) = 0;

	//! Compatibility wrapper, see resampler_manager_v2::make_chain().
	void make_chain_(dsp_chain_config& outChain, unsigned rateFrom, unsigned rateTo, float qualityScale);
};

class dsp_chain_config;

//! \since 1.6
class resampler_manager_v2 : public resampler_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(resampler_manager_v2, resampler_manager);
public:
	//! Make a chain of resamplers. \n
	//! Pass the intended sample rates for rateFrom & rateTo. Pass 0 rateFrom if it is not known in advance or may change in mid-conversion.
	//! With rateFrom known in advance, the chain should hold only one DSP. \n
	//! If rateFrom is not known in advance, multiple DSPs may be returned - a preferred one that accepts common conversion ratios but not all of them, and a fallback one that handles every scenario if the first one failed. \n
	//! For an example, by default, SSRC (higher quality) is used, but PPHS (more compatible) is added to clean up odd sample rates that SSRC failed to process. \n
	//! Note that it is required that resamplers pass untouched data if no resampling is performed, so additional DSPs have no effect on the audio coming thru, as just one resampler will actually do anything.
	virtual void make_chain( dsp_chain_config & outChain, unsigned rateFrom, unsigned rateTo, float qualityScale) = 0;
};

//! \since 1.6.1
class resampler_manager_v3 : public resampler_manager_v2 {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(resampler_manager_v3, resampler_manager_v2);
public:
	//! Extended make_chain that also manipulates channel layout.
	virtual void make_chain_v3(dsp_chain_config& outChain, unsigned rateFromm, unsigned rateTo, float qualityScale, unsigned chmask) = 0;
};
#endif

#endif // FOOBAR2000_HAVE_DSP
