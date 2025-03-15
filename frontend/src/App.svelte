<script>
	import Router from 'svelte-spa-router';
	import Home from './components/Home.svelte';
	import Login from './components/Login.svelte';
    import Signup from "./components/Signup.svelte";
    import Admin from "./components/Admin.svelte";
</script>

<Router routes={{
	'/': Home,
	'/login': Login,
	'/signup': Signup,
	'/admin': Admin,
}} />
