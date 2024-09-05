import { createEffect, createSignal, type Component } from 'solid-js';

enum Draw_Mode { RECTANGLE, BRUSH, SELECTION, TEXT };

const App: Component = () => {
	const [scale, setScale] = createSignal(100);
	const [mode, setMode] = createSignal(Draw_Mode.SELECTION);
	createEffect(() => {
		document.addEventListener("wheel", () => {
			const scale = Module.ccall("get_scale", "number", null, null);
			console.log("scale: ", scale);
			setScale(Math.round(scale * 100));
		});
	});
	return (
		<div class=''>
			<div
				class='fixed left-1/2 top-4 -translate-x-1/2 bg-bg text-white
				bg-primary p-2 rounded-md flex items-center gap-2'>
				<button class={`hover:bg-accent rounded-md`} onClick={() => {
					Module.ccall("clear", null, null, null);
				}}>
					<svg class="w-6 h-6 p-1" aria-hidden="true" role="img" viewBox="0 0 20 20" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><path stroke-width="1.25" d="M3.333 5.833h13.334M8.333 9.166v5M11.666 9.166v5M4.166 5.833l.833 10c0 .92.646 1.666 1.666 1.666h6.666c.92 0 1.666-.646 1.666-1.666l.833-10M6.5 5.833v-2.5c0-.46.363-.833.833-.833h3.334c.46 0 .833.363.833.833v2.5"></path></svg>
				</button>
				<button class={`${mode() == Draw_Mode.SELECTION ? "bg-accent" : ""} hover:bg-accent rounded-md`} onClick={() => {
					Module.ccall("set_mode", null, ["number"], [Draw_Mode.SELECTION]);
					setMode(Draw_Mode.SELECTION);
				}}>
					<svg class='w-6 h-6 p-1 ' aria-hidden="true" role="img" viewBox="0 0 22 22" fill="none" stroke-width="1.25"><g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><path stroke="none" d="M0 0h24v24H0z" fill="none"></path><path d="M6 6l4.153 11.793a0.365 .365 0 0 0 .331 .207a0.366 .366 0 0 0 .332 -.207l2.184 -4.793l4.787 -1.994a0.355 .355 0 0 0 .213 -.323a0.355 .355 0 0 0 -.213 -.323l-11.787 -4.36z"></path><path d="M13.5 13.5l4.5 4.5"></path></g></svg>
				</button>
				<button class={`${mode() == Draw_Mode.RECTANGLE ? "bg-accent" : ""} hover:bg-accent rounded-md`} onClick={() => {
					Module.ccall("set_mode", null, ["number"], [Draw_Mode.RECTANGLE]);
					setMode(Draw_Mode.RECTANGLE);
				}}>
					<svg class='w-6 h-6 p-1 ' aria-hidden="true" role="img" viewBox="0 0 24 24" fill="none" stroke-width="2" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><g stroke-width="1.5"><path stroke="none" d="M0 0h24v24H0z" fill="none"></path><rect x="4" y="4" width="16" height="16" rx="2"></rect></g></svg>
				</button>
				<button class={`${mode() == Draw_Mode.BRUSH ? "bg-accent" : ""} hover:bg-accent rounded-md`} onClick={() => {
					Module.ccall("set_mode", null, ["number"], [Draw_Mode.BRUSH]);
					setMode(Draw_Mode.BRUSH);
				}}>
					<svg class="w-6 h-6 p-1 " aria-hidden="true" role="img" viewBox="0 0 20 20" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><g stroke-width="1.25"><path clip-rule="evenodd" d="m6.643 15.69 6.664-6.663a2.356 2.356 0 1 0-3.334-3.334L4.31 12.356a3.333 3.333 0 0 0-.966 2.356v1.953h1.953c.884 0 1.632-.352 2.356-.966Z"></path><path d="m11.25 5.416 3.333 3.333"></path></g></svg>
				</button>
				<button class={`${mode() == Draw_Mode.TEXT ? "bg-accent" : ""} hover:bg-accent rounded-md`} onClick={() => {
					Module.ccall("clear", null, null, null);
				}}>
					<svg class="w-6 h-6 p-1" aria-hidden="true" role="img" viewBox="0 0 24 24" fill="none" stroke-width="2" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><g stroke-width="1.5"><path stroke="none" d="M0 0h24v24H0z" fill="none"></path><line x1="4" y1="20" x2="7" y2="20"></line><line x1="14" y1="20" x2="21" y2="20"></line><line x1="6.9" y1="15" x2="13.8" y2="15"></line><line x1="10.2" y1="6.3" x2="16" y2="20"></line><polyline points="5 20 11 4 13 4 20 20"></polyline></g></svg>
				</button>
				<div class='text-sm px-1'>
					{scale()}%
				</div>
			</div>
		</div>
	);
};

export default App;
