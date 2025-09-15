# otojsd

Otojs - live sound programming environment with JavaScript.

otojsd - sound processing server for Otojs.

Otojs は Javascript でリアルタイムサウンド生成を行うプログラム環境です。

otojsd はそのサウンド生成を担うサーバーです。

## What is this?

a javascript version of [OtoPerl](https://github.com/drumsoft/OtoPerl)'s otoperld.

The OtoPerl version produced sounds like these.

* https://soundcloud.com/otoperl
* https://www.youtube.com/watch?v=-ByATVMO658

You can generate these sound using JavaScript. Leveraging V8's speed, you should be able to do even more complex things.

## requirement.

* Mac OS

## Usage

Start otojsd.

```
release/otojsd
```

otojsd will start and listen 14609 (default port). When you send JavaScript code to otojsd, it compiles the code and uses it for sound generation.

```
curl -X POST http://localhost:14609/ --data-binary @examples/otojs-example.js
```

Using the attached otojsc script can reduce the amount typing.

```
./otojsc examples/otojs-example.js
```

* otojsd repeatedly calls the JavaScript function oto_render(frames, channels, input_array).
* oto_render must return a Float32Array with frames * channels elements.
* The returned Float32Array, for stereo, has samples for each channel arranged sequentially like [L0, R0, L1, R1, ...].
* When using otojsd's -i option, the sound input arrives in the input_array in a similar format.

otojsd_start.js, loaded when otojsd starts, contains the following oto_render function, which outputs a 440Hz sine wave as a test tone.

```
function oto_render(frames, channels, input_array) {
	let output = new Float32Array(frames * channels);
	for (let f = 0; f < frames; f++) {
		let v = 0.5 * Math.sin( 3.1415 * 2 * frame * 440 / sample_rate );
		for (let c = 0; c < channels; c++) {
			output[f * channels + c] = v;
		}
		frame++;
	}
	return output;
}
```

Overwriting this with another oto_render changes the output sound. This is an example performing AM modulation.

```
function oto_render(frames, channels, input_array) {
	let output = new Float32Array(frames * channels);
	for (let f = 0; f < frames; f++) {
		let v = 0.5 * Math.sin( 3.1415 * 2 * frame * 440 / sample_rate );
		let m = 0.5 + 0.5 * Math.sin( 3.1415 * 2 * frame * 1.0 / sample_rate );
		for (let c = 0; c < channels; c++) {
			output[f * channels + c] = v * m;
		}
		frame++;
	}
	return output;
}
```

Please also check the examples directory.

## launch options

otojsd supports all launch options from [otoperld](https://github.com/drumsoft/OtoPerl) (it should).

```
otojsd [-v] [-c channels] [-r sample_rate] [-a allowed_addresses] [-p port_number] [-i] [-d path/to/document_root] [filename]
 -v, --verbose       be verbose.
 -c, --channel 2     Number of channels otojsd generate. default is 2.
 -r, --rate 48000    Sampling rate of the sound otojsd generate. default is 48000.
 -a, --allow 192.168 Host address pattern allowed to access otojsd. default is 127.0.0.1
                     ex: 192.168.0.8 (1 host allowed, = 192.168.0.8/255.255.255.255)
                         192.168.1.0/255.255.252.0 (22 bit netmask)
                         192.168 (16 bit netmask, = 192.168.0.0/255.255.0.0)
 -p, --port 99999    Port number to listen. default is 14609.
 -f, --findfreeport  Find free port when it's already used. The found port will be put in file '.otojsd_port'.
 -o, --output x.aiff Record sounds to specified file.
 -i, --enable-input  Enables an audio input (from Default Input Device)
 -d, --document-root The path to the content returned when otojsd is accessed via GET method.
 filename            a Javascript file ran when server launched. default is 'otojsd-start.js'.
```

otojsc script has some options.

```
otojsc [-h host_address] [-p port_number|-f] filename
 -h 192.168.0.1      Host name to post. default is localhost.
 -p 99999            Port number to post. default is 14609.
 -f                  The port number will be read from '.otojsd_port' file.
 filename            a Javascript file sent to otojsd server.
                     if "-" (hyphen) specified, the content read from STDIN will be sent.
```

## build instruction

### build V8

Build libv8_monolith.a by referring to [Building V8 from source](https://v8.dev/docs/build) and [Getting started with embedding V8](https://v8.dev/docs/embed). Xcode and git are required.

clone depot tool from google.

```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Add depot_tools to your PATH.

```
export PATH=/path/to/depot_tools:"$PATH"
```

Get the V8 source code and enter the directory.

```
fetch v8
cd v8
```

Check out the [recommended version for embedding](https://v8.dev/docs/version-numbers#which-v8-version-should-i-use%3F).

```
git checkout -t branch-heads/14.0
```

Download dependencies.

```
gclient sync
```

Generate build configuration.

```
tools/dev/v8gen.py arm64.release.sample
```

Add `v8_enable_temporal_support = false` to build arguments.

```
gn args out.gn/arm64.release.sample
```

Build the libv8_monolith.a library.

```
ninja -C out.gn/arm64.release.sample v8_monolith
```

If the build succeeds, `out.gn/arm64.release.sample/obj/libv8_monolith.a` will be output to `/path/to/v8`.

### build otojsd

Requires cmake.

Generate build system. Specify the path where V8 was built for `-DV8_ROOT_DIR`. For debug builds, specify `-DCMAKE_BUILD_TYPE=Debug`.

```
cd /path/to/otojsd
cmake -B build -DV8_ROOT_DIR=/path/to/v8 -DCMAKE_BUILD_TYPE=Release
```

build.

```
cmake --build build
```

run.

```
build/otojsd
```
