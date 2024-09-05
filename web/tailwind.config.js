/** @type {import('tailwindcss').Config} */
module.exports = {
	content: [
		"./src/**/*.{js,jsx,ts,tsx}",
	],
	theme: {
		extend: {
			colors: {
				"background": "#181818",
				"primary": "#232329",
				"accent": "#403E6A",
			}
		},
	},
	plugins: [],
}

