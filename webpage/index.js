document.getElementById('login-form').addEventListener('submit', function(event) {
    event.preventDefault(); // Prevent the default form submission

    // Get the input values
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    // Simple client-side validation (you should also validate on the server-side)
    if (username === 'group12' && password === 'nas') {
        // Redirect to the desired page
        window.location.href = 'main/main.html'; 
    } else {
        document.getElementById('message').innerText = 'Invalid username or password.';
        document.getElementById('message').style.color = 'red';
    }
});
