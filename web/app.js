const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];

const state = {
  view: 'auth',
  category: 'Science',
  difficulty: 'Easy',
  user: null,
  csrfToken: null,
  question: null,
  answered: false,
  busy: false,
  score: 0,
  startedAt: 0,
  timer: null,
  pendingResult: null,
};

async function api(path, body) {
  const options = body === undefined ? { credentials: 'same-origin' } : {
    method: 'POST',
    credentials: 'same-origin',
    headers: {
      'Content-Type': 'application/json',
      ...(state.csrfToken ? { 'X-CSRF-Token': state.csrfToken } : {}),
    },
    body: JSON.stringify(body),
  };
  const response = await fetch(path, options);
  const data = await response.json();
  if (!response.ok) {
    if (response.status === 401 && state.user) {
      state.user = null;
      state.csrfToken = null;
      updateAvatar();
      setView('auth');
    }
    throw new Error(data.error || 'Something went wrong. Please try again.');
  }
  return data;
}

function toast(message) {
  const node = $('#toast');
  node.textContent = message;
  node.classList.remove('hidden');
  clearTimeout(toast.timeout);
  toast.timeout = setTimeout(() => node.classList.add('hidden'), 4200);
}

function setView(view) {
  if (!state.user && view !== 'auth') view = 'auth';
  state.view = view;
  document.body.classList.toggle('auth-mode', view === 'auth');
  if (view !== 'quiz') clearInterval(state.timer);
  $$('.view').forEach(node => node.classList.toggle('active', node.id === `${view}-view`));
  $$('.nav-link').forEach(node => node.classList.toggle('active', node.dataset.view === view));
  window.scrollTo({ top: 0, behavior: 'smooth' });
  if (view === 'leaderboard') loadLeaderboard();
  if (view === 'history') loadHistory();
  if (view === 'profile') loadProfile();
}

function setCategory(category) {
  state.category = category;
  $$('.category-card').forEach(node => {
    const selected = node.dataset.category === category;
    node.classList.toggle('selected', selected);
    node.setAttribute('aria-pressed', String(selected));
  });
  $('#random-category').textContent = category === 'Random'
    ? '✦  Random category selected'
    : '↝   Surprise me with a random category';
  updateSessionDetails();
}

function setDifficulty(difficulty) {
  state.difficulty = difficulty;
  $$('.difficulty-tabs button').forEach(node => {
    const selected = node.dataset.difficulty === difficulty;
    node.classList.toggle('selected', selected);
    node.setAttribute('aria-pressed', String(selected));
  });
  updateSessionDetails();
}

function updateSessionDetails() {
  $('#question-count').textContent = state.difficulty === 'Hard' ? '1'
    : state.difficulty === 'All' ? '5' : '2';
  $('#time-range').textContent = {
    Easy: '20 sec', Medium: '25 sec', Hard: '30 sec', All: '20–30 sec',
  }[state.difficulty];
}

function updateAvatar() {
  const name = state.user?.displayName || 'Guest';
  const initial = name[0].toUpperCase();
  $('#header-avatar').textContent = initial;
  $('#header-avatar').setAttribute('aria-label', `Open ${name}'s profile`);
  $('#session-player').textContent = name;
  $('#session-player-avatar').textContent = initial;
}

async function startQuiz() {
  if (state.busy || !state.user) return;
  state.busy = true;
  $('#start-quiz').disabled = true;
  try {
    const data = await api('/api/start', {
      category: state.category,
      difficulty: state.difficulty,
    });
    state.score = 0;
    state.pendingResult = null;
    $('#live-score').innerHTML = '0<span> pts</span>';
    $('#quiz-category-pill').textContent = `${data.category} · ${data.difficulty === 'All' ? 'Mixed' : data.difficulty}`;
    renderQuestion(data.question);
    setView('quiz');
  } catch (error) {
    toast(error.message);
  } finally {
    state.busy = false;
    $('#start-quiz').disabled = false;
  }
}

function formatTime(seconds) {
  return `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
}

function startTimer() {
  clearInterval(state.timer);
  state.startedAt = Date.now();
  $('#timer-display').textContent = '00:00';
  $('#timer-fill').style.width = '0%';
  state.timer = setInterval(() => {
    const seconds = Math.floor((Date.now() - state.startedAt) / 1000);
    $('#timer-display').textContent = formatTime(seconds);
    $('#timer-fill').style.width = `${Math.min(seconds / state.question.timeLimit * 100, 100)}%`;
  }, 250);
}

function renderQuestion(question) {
  state.question = question;
  state.answered = false;
  $('#quiz-heading').textContent = question.text;
  $('#quiz-progress-label').textContent = `QUESTION ${String(question.number).padStart(2, '0')} / ${String(question.total).padStart(2, '0')}`;
  $('#quiz-points-label').textContent = `+${question.points} POINTS`;
  $('#progress-fill').style.width = `${(question.number - 1) / question.total * 100}%`;
  $('#hint-text').classList.add('hidden');
  $('#feedback').className = 'feedback hidden';
  $('#hint-button').classList.remove('hidden');
  $('#hint-button').disabled = false;
  $('#skip-button').classList.remove('hidden');
  $('#next-button').classList.add('hidden');
  const container = $('#answer-options');
  container.replaceChildren();
  question.options.forEach((option, index) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'answer-option';
    button.dataset.letter = 'ABCD'[index];
    const letter = document.createElement('span');
    letter.className = 'answer-letter';
    letter.textContent = 'ABCD'[index];
    const value = document.createElement('span');
    value.textContent = option;
    button.append(letter, value);
    button.addEventListener('click', () => submitAnswer(button.dataset.letter));
    container.append(button);
  });
  startTimer();
}

async function revealHint() {
  if (state.busy || state.answered) return;
  state.busy = true;
  try {
    const data = await api('/api/hint', {});
    $('#hint-text').textContent = `Hint · ${data.hint}`;
    $('#hint-text').classList.remove('hidden');
    $('#hint-button').disabled = true;
  } catch (error) {
    toast(error.message);
  } finally {
    state.busy = false;
  }
}

async function submitAnswer(answer) {
  if (state.busy || state.answered) return;
  state.busy = true;
  try {
    const data = await api('/api/answer', { answer });
    clearInterval(state.timer);
    state.answered = true;
    const { feedback } = data;
    state.score += feedback.points;
    $('#live-score').innerHTML = `${state.score}<span> pts</span>`;
    $$('.answer-option').forEach(node => {
      node.disabled = true;
      if (node.dataset.letter === feedback.correctLetter) node.classList.add('correct');
      else if (node.dataset.letter === answer && feedback.outcome !== 'skipped') node.classList.add('wrong');
    });
    const message = {
      correct: `✓ Correct! +${feedback.points} points${feedback.bonus ? ` · includes a ${feedback.bonus} point time bonus` : ''}`,
      wrong: `✕ Not quite. The correct answer is ${feedback.correctLetter}. ${feedback.correctText}.`,
      skipped: `Skipped. The correct answer was ${feedback.correctLetter}. ${feedback.correctText}.`,
      invalid: `Invalid answer — marked wrong. The correct answer is ${feedback.correctLetter}. ${feedback.correctText}.`,
    }[feedback.outcome];
    $('#feedback').textContent = message;
    $('#feedback').className = `feedback ${feedback.outcome}`;
    $('#hint-button').classList.add('hidden');
    $('#skip-button').classList.add('hidden');
    const next = $('#next-button');
    next.innerHTML = data.done ? 'View results <span aria-hidden="true">→</span>' : 'Next question <span aria-hidden="true">→</span>';
    next.classList.remove('hidden');
    state.pendingResult = data.result || null;
    $('#progress-fill').style.width = `${state.question.number / state.question.total * 100}%`;
  } catch (error) {
    toast(error.message);
  } finally {
    state.busy = false;
  }
}

async function nextQuestion() {
  if (state.busy) return;
  if (state.pendingResult) {
    showResults(state.pendingResult);
    return;
  }
  state.busy = true;
  try {
    const data = await api('/api/next', {});
    renderQuestion(data.question);
  } catch (error) {
    toast(error.message);
  } finally {
    state.busy = false;
  }
}

function showResults(result) {
  const labels = { S: 'Outstanding!', A: 'Excellent work.', B: 'Nicely done.', C: 'Keep it going.', D: 'A fresh start awaits.' };
  $('#result-grade').textContent = result.grade;
  $('#result-title').textContent = labels[result.grade];
  $('#result-subtitle').textContent = `${state.user.displayName}, you completed the ${result.category} quiz on ${result.difficulty === 'All' ? 'Mixed' : result.difficulty} difficulty.`;
  $('#result-score').textContent = `${result.score} pts`;
  $('#result-accuracy').textContent = `${result.percentage.toFixed(1)}%`;
  $('#result-correct').textContent = `${result.correct} / ${result.total}`;
  $('#save-status').textContent = result.saved ? '✓ Your result is saved to your account.' : 'Your result could not be saved. Please try again.';
  setView('results');
}

function cell(className, text) {
  const node = document.createElement('span');
  node.className = className;
  node.textContent = text;
  return node;
}

function emptyState(target, title, description) {
  const wrapper = document.createElement('div');
  wrapper.className = 'empty-state';
  const heading = document.createElement('strong');
  heading.textContent = title;
  const copy = document.createElement('span');
  copy.textContent = description;
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'button button-primary';
  button.textContent = 'Start a quiz →';
  button.addEventListener('click', () => setView('home'));
  wrapper.append(heading, copy, document.createElement('br'), button);
  target.replaceChildren(wrapper);
}

function renderRows(target, entries, history = false) {
  target.replaceChildren();
  entries.forEach((entry, index) => {
    const row = document.createElement('div');
    row.className = 'table-row';
    row.append(
      cell(history ? 'player' : 'rank', history ? `#${index + 1}` : ['🥇', '🥈', '🥉'][index] || `#${index + 1}`),
      cell(history ? 'score' : 'player', history ? `${entry.score} pts` : entry.name),
      cell(history ? '' : 'score', history ? `${entry.correctAnswers}/${entry.totalQuestions}` : `${entry.score} pts`),
      cell('', history ? `${entry.percentage.toFixed(1)}%` : `${entry.percentage.toFixed(1)}%`),
      cell('badge', entry.difficulty === 'All' ? 'Mixed' : entry.difficulty),
      cell('', entry.date),
    );
    target.append(row);
  });
}

async function loadLeaderboard() {
  const target = $('#leaderboard-rows');
  target.textContent = 'Loading scores…';
  try {
    const data = await api('/api/leaderboard');
    if (data.entries.length) renderRows(target, data.entries);
    else emptyState(target, 'No scores yet.', 'Be the first to claim a place on the leaderboard.');
  } catch (error) {
    emptyState(target, 'Scores unavailable.', error.message);
  }
}

async function loadHistory() {
  const target = $('#history-rows');
  $('#history-intro').textContent = `Showing saved sessions for ${state.user.displayName}.`;
  target.textContent = 'Loading history…';
  try {
    const data = await api('/api/history');
    const entries = data.entries;
    const totalPoints = entries.reduce((sum, entry) => sum + entry.score, 0);
    const stats = [
      ['GAMES PLAYED', entries.length],
      ['TOTAL POINTS', totalPoints],
      ['AVERAGE POINTS', entries.length ? (totalPoints / entries.length).toFixed(1) : '0.0'],
    ];
    $('#history-stats').replaceChildren(...stats.map(([label, value]) => {
      const wrapper = document.createElement('div');
      wrapper.className = 'history-stat';
      wrapper.append(cell('', label));
      const strong = document.createElement('strong');
      strong.textContent = String(value);
      wrapper.append(strong);
      return wrapper;
    }));
    if (entries.length) renderRows(target, entries, true);
    else emptyState(target, 'No history yet.', 'Complete a quiz to start your personal record.');
  } catch (error) {
    emptyState(target, 'History unavailable.', error.message);
  }
}

function setAuthTab(mode) {
  const register = mode === 'register';
  $('#login-tab').classList.toggle('active', !register);
  $('#register-tab').classList.toggle('active', register);
  $('#login-tab').setAttribute('aria-selected', String(!register));
  $('#register-tab').setAttribute('aria-selected', String(register));
  $('#register-fields').classList.toggle('hidden', !register);
  $('#auth-display-name').required = register;
  $('#auth-password').autocomplete = register ? 'new-password' : 'current-password';
  $('#auth-heading').textContent = register ? 'Start your journey.' : 'Welcome back.';
  $('#auth-intro').textContent = register
    ? 'Make an account and turn every quiz into progress.'
    : 'Pick up right where your curiosity left off.';
  $('#auth-submit').innerHTML = register
    ? 'Create account <span aria-hidden="true">→</span>'
    : 'Sign in <span aria-hidden="true">→</span>';
  $('#auth-error').textContent = '';
  $('#auth-form').dataset.mode = mode;
}

async function submitAuth(event) {
  event.preventDefault();
  if (state.busy) return;
  const register = $('#auth-form').dataset.mode === 'register';
  const username = $('#auth-username').value.trim();
  const password = $('#auth-password').value;
  const payload = { username, password };
  if (register) payload.displayName = $('#auth-display-name').value.trim();
  state.busy = true;
  $('#auth-submit').disabled = true;
  $('#auth-error').textContent = '';
  try {
    const data = await api(register ? '/api/register' : '/api/login', payload);
    state.user = data.user;
    state.csrfToken = data.csrfToken;
    $('#auth-password').value = '';
    updateAvatar();
    setView('home');
  } catch (error) {
    $('#auth-error').textContent = error.message;
  } finally {
    state.busy = false;
    $('#auth-submit').disabled = false;
  }
}

function profileStat(label, value) {
  const wrapper = document.createElement('div');
  wrapper.className = 'profile-stat';
  wrapper.append(cell('', label));
  const strong = document.createElement('strong');
  strong.textContent = String(value);
  wrapper.append(strong);
  return wrapper;
}

async function loadProfile() {
  try {
    const data = await api('/api/profile');
    state.user = data.user;
    updateAvatar();
    $('#profile-avatar').textContent = data.user.displayName[0].toUpperCase();
    $('#profile-name').textContent = data.user.displayName;
    $('#profile-username').textContent = `@${data.user.username}`;
    $('#profile-joined').textContent = `Member since ${data.user.joined.split(' ')[0]}`;
    $('#profile-display-name').value = data.user.displayName;
    $('#profile-stats').replaceChildren(
      profileStat('GAMES PLAYED', data.stats.games),
      profileStat('TOTAL POINTS', data.stats.points),
      profileStat('BEST SCORE', data.stats.best),
      profileStat('AVG POINTS', data.stats.average.toFixed(1)),
    );
    const recent = $('#profile-recent-list');
    recent.replaceChildren();
    if (!data.recent.length) {
      const empty = document.createElement('p');
      empty.className = 'profile-empty';
      empty.textContent = 'No sessions yet. Your first quiz is waiting.';
      recent.append(empty);
    }
    data.recent.forEach(entry => {
      const row = document.createElement('div');
      row.className = 'recent-row';
      const title = document.createElement('div');
      const strong = document.createElement('strong');
      strong.textContent = `${entry.category} · ${entry.difficulty === 'All' ? 'Mixed' : entry.difficulty}`;
      const date = document.createElement('small');
      date.textContent = entry.date;
      title.append(strong, date);
      const score = cell('recent-score', `${entry.score} pts`);
      const grade = cell('recent-grade', gradeFor(entry.percentage));
      row.append(title, score, grade);
      recent.append(row);
    });
  } catch (error) {
    toast(error.message);
  }
}

function gradeFor(percentage) {
  if (percentage >= 90) return 'S';
  if (percentage >= 75) return 'A';
  if (percentage >= 60) return 'B';
  if (percentage >= 40) return 'C';
  return 'D';
}

async function saveProfile(event) {
  event.preventDefault();
  if (state.busy) return;
  const payload = { displayName: $('#profile-display-name').value.trim() };
  const currentPassword = $('#current-password').value;
  const newPassword = $('#new-password').value;
  if (currentPassword || newPassword) {
    payload.currentPassword = currentPassword;
    payload.newPassword = newPassword;
  }
  state.busy = true;
  $('#profile-save').disabled = true;
  $('#profile-error').textContent = '';
  try {
    const data = await api('/api/profile', payload);
    state.user = data.user;
    updateAvatar();
    $('#current-password').value = '';
    $('#new-password').value = '';
    toast('Profile updated.');
    loadProfile();
  } catch (error) {
    $('#profile-error').textContent = error.message;
  } finally {
    state.busy = false;
    $('#profile-save').disabled = false;
  }
}

async function logout() {
  if (state.busy) return;
  state.busy = true;
  try {
    await api('/api/logout', {});
    state.user = null;
    state.csrfToken = null;
    state.pendingResult = null;
    clearInterval(state.timer);
    $('#auth-username').value = '';
    $('#auth-password').value = '';
    $('#auth-display-name').value = '';
    setAuthTab('login');
    updateAvatar();
    setView('auth');
  } catch (error) {
    toast(error.message);
  } finally {
    state.busy = false;
  }
}

async function boot() {
  setAuthTab('login');
  try {
    const data = await api('/api/me');
    state.user = data.user;
    state.csrfToken = data.csrfToken;
    updateAvatar();
    setView('home');
  } catch {
    setView('auth');
  }
}

$$('[data-view]').forEach(node => node.addEventListener('click', () => setView(node.dataset.view)));
$$('.category-card').forEach(node => node.addEventListener('click', () => setCategory(node.dataset.category)));
$$('.difficulty-tabs button').forEach(node => node.addEventListener('click', () => setDifficulty(node.dataset.difficulty)));
$('#random-category').addEventListener('click', () => setCategory(state.category === 'Random' ? 'Science' : 'Random'));
$('#hero-start').addEventListener('click', () => $('#setup').scrollIntoView({ behavior: 'smooth' }));
$('#start-quiz').addEventListener('click', startQuiz);
$('#login-tab').addEventListener('click', () => setAuthTab('login'));
$('#register-tab').addEventListener('click', () => setAuthTab('register'));
$('#auth-form').addEventListener('submit', submitAuth);
$('#profile-form').addEventListener('submit', saveProfile);
$('#logout-button').addEventListener('click', logout);
$('.brand').addEventListener('click', event => { event.preventDefault(); setView('home'); });
$('#hint-button').addEventListener('click', revealHint);
$('#skip-button').addEventListener('click', () => submitAnswer('S'));
$('#next-button').addEventListener('click', nextQuestion);
$('#play-again').addEventListener('click', () => setView('home'));
document.addEventListener('keydown', event => {
  if (state.view !== 'quiz' || state.answered || state.busy || /INPUT|TEXTAREA/.test(document.activeElement.tagName)) return;
  const key = event.key.toUpperCase();
  if ('ABCD'.includes(key) && key.length === 1) submitAnswer(key);
  else if (key === 'H') revealHint();
  else if (key === 'S') submitAnswer('S');
});
updateSessionDetails();
boot();
