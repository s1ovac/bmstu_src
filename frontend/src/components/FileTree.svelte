<script>
    import { onMount, onDestroy } from 'svelte';
    import { writable, get } from 'svelte/store';
    import {
        getFileTree,
        getFolders,
        uploadFile,
        deleteFiles,
        downloadFile,
        createFolder,
        deleteFolder,
        moveFile
    } from '../lib/api.js';

    // Check for browser environment to prevent SSR issues
    const isBrowser = typeof window !== 'undefined';

    const files = writable([]);
    const folders = writable([]);
    const error = writable(null);
    const success = writable(null);
    const loading = writable(false);
    const selectedItems = writable([]); // Для хранения выбранных файлов и папок
    const isSelectionMode = writable(false);
    const showFolderModal = writable(false);
    const showDeleteConfirm = writable(false);

    let uploadInput;
    let newFolderName = '';
    let currentFolderId = 0; // Корневая папка
    let folderPath = []; // Для отслеживания пути навигации
    let searchQuery = '';
    let filteredFiles = [];
    let filteredFolders = [];
    let messageTimeout;
    let targetMoveFolder = null;
    let showMoveModal = false;

    // Get settings from localStorage or use defaults
    let viewMode = isBrowser ? (localStorage.getItem('viewMode') || 'grid') : 'grid'; // 'grid' или 'list'
    let sortBy = isBrowser ? (localStorage.getItem('sortBy') || 'name') : 'name'; // 'name', 'date', 'size'
    let sortOrder = isBrowser ? (localStorage.getItem('sortOrder') || 'asc') : 'asc'; // 'asc' или 'desc'

    // Экспортируем метод для обновления данных извне
    export function refreshData() {
        fetchData();
    }

    // Export method to trigger file upload
    export function triggerFileUpload() {
        if (uploadInput) {
            uploadInput.click();
        }
    }

    // Export method to open create folder dialog
    export function openCreateFolderDialog() {
        openCreateFolderModal();
    }

    // Export method for searching files
    export function searchFiles(query) {
        searchQuery = query;
        handleSearchInput();
    }

    // Export method to clear search
    export function clearSearch() {
        searchQuery = '';
        handleSearchInput();
    }

    onMount(async () => {
        $loading = true;
        await fetchData();

        // Добавляем слушатель клавиатуры для отмены выбора по Escape
        if (isBrowser) {
            document.addEventListener('keydown', handleKeyDown);
        }
    });

    onDestroy(() => {
        // Убираем слушатель при уничтожении компонента
        if (isBrowser) {
            document.removeEventListener('keydown', handleKeyDown);
        }

        // Очищаем таймауты
        if (messageTimeout) {
            clearTimeout(messageTimeout);
        }
    });

    function handleKeyDown(e) {
        if (e.key === 'Escape') {
            if ($showFolderModal || $showDeleteConfirm || showMoveModal) {
                $showFolderModal = false;
                $showDeleteConfirm = false;
                showMoveModal = false;
            } else if ($isSelectionMode) {
                $selectedItems = [];
                $isSelectionMode = false;
            }
        }
    }

    async function fetchData() {
        $loading = true;
        $error = null;
        try {
            if (!isBrowser) {
                // If not in browser, just end loading
                $loading = false;
                return;
            }

            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            const [folderResponse, fileResponse] = await Promise.all([
                getFolders(token, currentFolderId),
                getFileTree(token, currentFolderId)
            ]);

            $folders = folderResponse.folders || [];
            $files = fileResponse.files || [];

            // Применяем фильтрацию и сортировку
            applyFilterAndSort();
        } catch (err) {
            console.error('Error fetching data:', err);
            $error = err.message || 'Ошибка при загрузке данных';
        } finally {
            $loading = false;
        }
    }

    function applyFilterAndSort() {
        // Фильтрация
        if (searchQuery) {
            const query = searchQuery.toLowerCase();
            filteredFiles = get(files).filter(file =>
                file.file_name.toLowerCase().includes(query)
            );
            filteredFolders = get(folders).filter(folder =>
                folder.folder_name.toLowerCase().includes(query)
            );
        } else {
            filteredFiles = [...get(files)];
            filteredFolders = [...get(folders)];
        }

        // Сортировка
        switch (sortBy) {
            case 'name':
                filteredFolders.sort((a, b) => {
                    const nameA = a.folder_name.toLowerCase();
                    const nameB = b.folder_name.toLowerCase();
                    return sortOrder === 'asc'
                        ? nameA.localeCompare(nameB)
                        : nameB.localeCompare(nameA);
                });

                filteredFiles.sort((a, b) => {
                    const nameA = a.file_name.toLowerCase();
                    const nameB = b.file_name.toLowerCase();
                    return sortOrder === 'asc'
                        ? nameA.localeCompare(nameB)
                        : nameB.localeCompare(nameA);
                });
                break;

            case 'date':
                filteredFolders.sort((a, b) => {
                    const dateA = new Date(a.created_at);
                    const dateB = new Date(b.created_at);
                    return sortOrder === 'asc'
                        ? dateA - dateB
                        : dateB - dateA;
                });

                filteredFiles.sort((a, b) => {
                    const dateA = new Date(a.created_at);
                    const dateB = new Date(b.created_at);
                    return sortOrder === 'asc'
                        ? dateA - dateB
                        : dateB - dateA;
                });
                break;

            case 'size':
                // Сортируем только файлы по размеру, папки остаются отсортированными по имени
                filteredFolders.sort((a, b) => {
                    const nameA = a.folder_name.toLowerCase();
                    const nameB = b.folder_name.toLowerCase();
                    return sortOrder === 'asc'
                        ? nameA.localeCompare(nameB)
                        : nameB.localeCompare(nameA);
                });

                filteredFiles.sort((a, b) => {
                    return sortOrder === 'asc'
                        ? a.file_size - b.file_size
                        : b.file_size - a.file_size;
                });
                break;
        }
    }

    function setSortBy(field) {
        if (sortBy === field) {
            // Если текущее поле сортировки совпадает, меняем направление
            sortOrder = sortOrder === 'asc' ? 'desc' : 'asc';
        } else {
            // Иначе устанавливаем новое поле и сбрасываем направление на 'asc'
            sortBy = field;
            sortOrder = 'asc';
        }

        // Сохраняем в localStorage (только если в браузере)
        if (isBrowser) {
            localStorage.setItem('sortBy', sortBy);
            localStorage.setItem('sortOrder', sortOrder);
        }

        applyFilterAndSort();
    }

    function setViewMode(mode) {
        viewMode = mode;
        if (isBrowser) {
            localStorage.setItem('viewMode', mode);
        }
    }

    function showMessage(type, msg) {
        if (type === 'error') {
            $error = msg;
            $success = null;
        } else {
            $success = msg;
            $error = null;
        }

        // Автоматически скрываем сообщение через 5 секунд
        if (messageTimeout) {
            clearTimeout(messageTimeout);
        }

        messageTimeout = setTimeout(() => {
            $error = null;
            $success = null;
        }, 5000);
    }

    function handleSearchInput() {
        applyFilterAndSort();
    }

    function handleUploadClick() {
        if (uploadInput) {
            uploadInput.click();
        }
    }

    async function handleUpload() {
        if (!isBrowser) return;

        const filesToUpload = uploadInput.files;
        if (filesToUpload.length === 0) {
            showMessage('error', 'Не выбран файл для загрузки');
            return;
        }

        $loading = true;
        try {
            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            // Загружаем все выбранные файлы
            const uploadPromises = Array.from(filesToUpload).map(file =>
                uploadFile(token, currentFolderId, file)
            );

            await Promise.all(uploadPromises);

            showMessage('success', 'Файлы успешно загружены');
            uploadInput.value = '';
            await fetchData();
        } catch (err) {
            showMessage('error', `Ошибка загрузки: ${err.message}`);
        } finally {
            $loading = false;
        }
    }

    function openCreateFolderModal() {
        newFolderName = '';
        $showFolderModal = true;
    }

    async function handleCreateFolder() {
        if (!isBrowser) return;

        if (!newFolderName.trim()) {
            showMessage('error', 'Введите название папки');
            return;
        }

        $loading = true;
        try {
            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            await createFolder(token, newFolderName, currentFolderId);

            showMessage('success', `Папка "${newFolderName}" создана`);
            newFolderName = '';
            $showFolderModal = false;
            await fetchData();
        } catch (err) {
            showMessage('error', `Ошибка создания папки: ${err.message}`);
        } finally {
            $loading = false;
        }
    }

    function confirmDelete() {
        if ($selectedItems.length === 0) {
            showMessage('error', 'Не выбраны элементы для удаления');
            return;
        }
        $showDeleteConfirm = true;
    }

    async function handleDelete() {
        if (!isBrowser) return;

        if ($selectedItems.length === 0) {
            showMessage('error', 'Не выбраны элементы для удаления');
            return;
        }

        $loading = true;
        try {
            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            const fileIds = $selectedItems
                .filter(item => item.type === 'file')
                .map(item => item.id);

            const folderIds = $selectedItems
                .filter(item => item.type === 'folder')
                .map(item => item.id);

            const promises = [];

            if (fileIds.length > 0) {
                promises.push(deleteFiles(token, fileIds));
            }

            if (folderIds.length > 0) {
                folderIds.forEach(id => {
                    promises.push(deleteFolder(token, id));
                });
            }

            await Promise.all(promises);

            showMessage('success', 'Выбранные элементы удалены');
            $selectedItems = [];
            $isSelectionMode = false;
            $showDeleteConfirm = false;
            await fetchData();
        } catch (err) {
            showMessage('error', `Ошибка удаления: ${err.message}`);
        } finally {
            $loading = false;
        }
    }

    async function handleDownload(fileId, fileName) {
        if (!isBrowser) return;

        $loading = true;
        try {
            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            const blob = await downloadFile(token, fileId);

            // Создаем ссылку для скачивания
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = fileName;
            document.body.appendChild(a);
            a.click();
            a.remove();

            // Освобождаем URL
            window.URL.revokeObjectURL(url);

            showMessage('success', `Файл "${fileName}" скачан`);
        } catch (err) {
            showMessage('error', `Ошибка скачивания: ${err.message}`);
        } finally {
            $loading = false;
        }
    }

    function navigateToFolder(folderId, folderName) {
        if ($isSelectionMode) {
            toggleItemSelection({id: folderId, type: 'folder'});
            return;
        }

        // Добавляем текущую папку в путь
        if (currentFolderId !== 0) {
            const currentFolderInfo = folderPath.find(f => f.id === currentFolderId);
            if (!currentFolderInfo) {
                // Если информация о текущей папке отсутствует в пути, добавляем её
                const currentName = get(folders).find(f => f.folder_id === currentFolderId)?.folder_name || 'Неизвестная папка';
                folderPath = [...folderPath, {id: currentFolderId, name: currentName}];
            }
        }

        // Устанавливаем новую текущую папку
        currentFolderId = folderId;
        fetchData();
    }

    function navigateBack() {
        if (folderPath.length > 0) {
            // Извлекаем последнюю папку из пути
            const previousFolder = folderPath.pop();
            currentFolderId = previousFolder.id;
            fetchData();
        }
    }

    function navigateToRoot() {
        folderPath = [];
        currentFolderId = 0;
        fetchData();
    }

    function navigateToPathIndex(index) {
        if (index === -1) {
            // Корневая директория
            navigateToRoot();
            return;
        }

        // Обрезаем путь до указанного индекса
        folderPath = folderPath.slice(0, index + 1);
        currentFolderId = folderPath[index].id;
        fetchData();
    }

    function toggleItemSelection(item) {
        const index = $selectedItems.findIndex(selected =>
            selected.id === item.id && selected.type === item.type
        );

        if (index !== -1) {
            // Удаляем элемент из выбранных
            $selectedItems = $selectedItems.filter((_, i) => i !== index);

            // Если больше нет выбранных элементов, выходим из режима выбора
            if ($selectedItems.length === 0) {
                $isSelectionMode = false;
            }
        } else {
            // Добавляем элемент в выбранные
            $selectedItems = [...$selectedItems, item];

            // Включаем режим выбора, если он еще не включен
            if (!$isSelectionMode) {
                $isSelectionMode = true;
            }
        }
    }

    function selectAllItems() {
        if (filteredFiles.length === 0 && filteredFolders.length === 0) {
            return;
        }

        const allItems = [
            ...filteredFolders.map(folder => ({id: folder.folder_id, type: 'folder'})),
            ...filteredFiles.map(file => ({id: file.file_id, type: 'file'}))
        ];

        $selectedItems = allItems;
        $isSelectionMode = true;
    }

    function clearSelection() {
        $selectedItems = [];
        $isSelectionMode = false;
    }

    function openMoveDialog() {
        if ($selectedItems.length === 0) {
            showMessage('error', 'Не выбраны элементы для перемещения');
            return;
        }

        targetMoveFolder = null;
        showMoveModal = true;
    }

    function selectTargetFolder(folder) {
        targetMoveFolder = folder;
    }

    async function handleMoveItems() {
        if (!isBrowser) return;

        if (!targetMoveFolder) {
            showMessage('error', 'Не выбрана целевая папка');
            return;
        }

        $loading = true;
        try {
            const userData = JSON.parse(localStorage.getItem('user') || '{}');
            const token = userData?.token;

            if (!token) {
                throw new Error('Пожалуйста, войдите в систему');
            }

            // Перемещаем файлы
            const fileIds = $selectedItems
                .filter(item => item.type === 'file')
                .map(item => item.id);

            if (fileIds.length > 0) {
                // Вызываем API для перемещения файлов
                await moveFile(token, fileIds, targetMoveFolder.folder_id);
            }

            // Обновляем данные
            showMessage('success', 'Элементы перемещены');
            $selectedItems = [];
            $isSelectionMode = false;
            showMoveModal = false;
            await fetchData();
        } catch (err) {
            showMessage('error', `Ошибка перемещения: ${err.message}`);
        } finally {
            $loading = false;
        }
    }

    // Вспомогательная функция для форматирования размера файла
    function formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';

        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // Вспомогательная функция для форматирования даты
    function formatDate(dateString) {
        const date = new Date(dateString);
        return date.toLocaleString('ru-RU', {
            year: 'numeric',
            month: 'short',
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit'
        });
    }

    // Определяем иконку для файла на основе его расширения
    function getFileIcon(fileName) {
        const extension = fileName.split('.').pop().toLowerCase();

        const iconMap = {
            pdf: 'picture_as_pdf',
            doc: 'description',
            docx: 'description',
            xls: 'table_chart',
            xlsx: 'table_chart',
            ppt: 'slideshow',
            pptx: 'slideshow',
            jpg: 'image',
            jpeg: 'image',
            png: 'image',
            gif: 'image',
            svg: 'image',
            mp3: 'audiotrack',
            wav: 'audiotrack',
            mp4: 'movie',
            avi: 'movie',
            mov: 'movie',
            zip: 'archive',
            rar: 'archive',
            txt: 'article',
            html: 'code',
            css: 'code',
            js: 'code',
            json: 'code',
            xml: 'code'
        };

        return iconMap[extension] || 'insert_drive_file';
    }

    // Получаем класс цвета для иконки файла на основе расширения
    function getFileColorClass(fileName) {
        const extension = fileName.split('.').pop().toLowerCase();

        const colorMap = {
            pdf: 'file-icon-pdf',
            doc: 'file-icon-doc',
            docx: 'file-icon-doc',
            xls: 'file-icon-sheet',
            xlsx: 'file-icon-sheet',
            ppt: 'file-icon-slide',
            pptx: 'file-icon-slide',
            jpg: 'file-icon-image',
            jpeg: 'file-icon-image',
            png: 'file-icon-image',
            gif: 'file-icon-image',
            svg: 'file-icon-image',
            mp3: 'file-icon-audio',
            wav: 'file-icon-audio',
            mp4: 'file-icon-video',
            avi: 'file-icon-video',
            mov: 'file-icon-video',
            zip: 'file-icon-archive',
            rar: 'file-icon-archive',
            txt: 'file-icon-text',
            html: 'file-icon-code',
            css: 'file-icon-code',
            js: 'file-icon-code',
            json: 'file-icon-code',
            xml: 'file-icon-code'
        };

        return colorMap[extension] || 'file-icon-default';
    }
</script>

<!-- Верхняя панель с действиями и хлебными крошками -->
<div class="file-manager-header">
    <div class="breadcrumb-container">
        <button class="icon-button" title="Назад" disabled={folderPath.length === 0} on:click={navigateBack}>
            <i class="material-icons">arrow_back</i>
        </button>

        <div class="breadcrumb">
            <span
                    class="breadcrumb-item"
                    class:active={currentFolderId === 0}
                    on:click={() => navigateToPathIndex(-1)}
            >
                <i class="material-icons">cloud</i>
                Мой диск
            </span>

            {#each folderPath as folder, index}
                <i class="material-icons breadcrumb-separator">chevron_right</i>
                <span
                        class="breadcrumb-item"
                        class:active={index === folderPath.length - 1}
                        on:click={() => navigateToPathIndex(index)}
                >
                    {folder.name}
                </span>
            {/each}
        </div>
    </div>

    <div class="search-container">
        <div class="search-input-wrapper">
            <i class="material-icons">search</i>
            <input
                    type="text"
                    placeholder="Поиск"
                    bind:value={searchQuery}
                    on:input={handleSearchInput}
            />
            {#if searchQuery}
                <button class="clear-search-button" on:click={() => { searchQuery = ''; handleSearchInput(); }}>
                    <i class="material-icons">close</i>
                </button>
            {/if}
        </div>
    </div>
</div>

<!-- Панель инструментов -->
<div class="toolbar">
    <div class="left-actions">
        {#if $isSelectionMode}
            <div class="selection-info">
                <span>Выбрано: {$selectedItems.length}</span>
                <button class="text-button" on:click={clearSelection}>
                    Отменить выбор
                </button>
            </div>

            <div class="selection-actions">
                <button class="icon-button" title="Удалить" on:click={confirmDelete}>
                    <i class="material-icons">delete</i>
                </button>
                <button class="icon-button" title="Переместить" on:click={openMoveDialog}>
                    <i class="material-icons">drive_file_move</i>
                </button>
            </div>
        {:else}
            <div class="primary-actions">
                <button class="upload-button" on:click={handleUploadClick}>
                    <i class="material-icons">upload_file</i>
                    <span>Загрузить</span>
                </button>
                <button class="icon-button" title="Создать папку" on:click={openCreateFolderModal}>
                    <i class="material-icons">create_new_folder</i>
                </button>
            </div>
        {/if}
    </div>

    <div class="right-actions">
        <div class="view-options">
            <button
                    class="icon-button"
                    class:active={viewMode === 'grid'}
                    title="Сетка"
                    on:click={() => setViewMode('grid')}
            >
                <i class="material-icons">grid_view</i>
            </button>
            <button
                    class="icon-button"
                    class:active={viewMode === 'list'}
                    title="Список"
                    on:click={() => setViewMode('list')}
            >
                <i class="material-icons">view_list</i>
            </button>
        </div>

        <div class="sort-options">
            <button
                    class="sort-button"
                    class:active={sortBy === 'name'}
                    on:click={() => setSortBy('name')}
            >
                <span>Имя</span>
                {#if sortBy === 'name'}
                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                {/if}
            </button>

            <button
                    class="sort-button"
                    class:active={sortBy === 'date'}
                    on:click={() => setSortBy('date')}
            >
                <span>Дата</span>
                {#if sortBy === 'date'}
                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                {/if}
            </button>

            <button
                    class="sort-button"
                    class:active={sortBy === 'size'}
                    on:click={() => setSortBy('size')}
            >
                <span>Размер</span>
                {#if sortBy === 'size'}
                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                {/if}
            </button>
        </div>
    </div>
</div>

<!-- Невидимый input для загрузки файлов -->
<input
        type="file"
        style="display: none"
        bind:this={uploadInput}
        on:change={handleUpload}
        multiple
/>

<!-- Сообщения -->
{#if $error}
    <div class="message error-message">
        <i class="material-icons">error</i>
        <span>{$error}</span>
        <button on:click={() => $error = null}>
            <i class="material-icons">close</i>
        </button>
    </div>
{/if}

{#if $success}
    <div class="message success-message">
        <i class="material-icons">check_circle</i>
        <span>{$success}</span>
        <button on:click={() => $success = null}>
            <i class="material-icons">close</i>
        </button>
    </div>
{/if}

<!-- Основной контент -->
<div class="file-manager-content">
    {#if $loading && filteredFiles.length === 0 && filteredFolders.length === 0}
        <div class="loading-container">
            <div class="loader"></div>
            <span>Загрузка...</span>
        </div>
    {:else if filteredFiles.length === 0 && filteredFolders.length === 0}
        <div class="empty-state">
            {#if searchQuery}
                <i class="material-icons">search_off</i>
                <h3>Ничего не найдено</h3>
                <p>По запросу "{searchQuery}" ничего не найдено</p>
                <button class="text-button" on:click={() => { searchQuery = ''; handleSearchInput(); }}>
                    Очистить поиск
                </button>
            {:else}
                <i class="material-icons">folder_off</i>
                <h3>Папка пуста</h3>
                <p>В этой папке пока нет файлов и подпапок</p>
                <div class="empty-actions">
                    <button class="primary-button" on:click={handleUploadClick}>
                        <i class="material-icons">upload_file</i>
                        <span>Загрузить файлы</span>
                    </button>
                    <button class="secondary-button" on:click={openCreateFolderModal}>
                        <i class="material-icons">create_new_folder</i>
                        <span>Создать папку</span>
                    </button>
                </div>
            {/if}
        </div>
    {:else}
        <!-- Режим отображения: сетка или список -->
        {#if viewMode === 'grid'}
            <div class="grid-view">
                {#if !searchQuery}
                    <!-- Папки всегда отображаются первыми -->
                    {#each filteredFolders as folder}
                        <div
                                class="grid-item folder-item"
                                class:selected={$selectedItems.some(item => item.id === folder.folder_id && item.type === 'folder')}
                                on:click={() => $isSelectionMode
                                ? toggleItemSelection({id: folder.folder_id, type: 'folder'})
                                : navigateToFolder(folder.folder_id, folder.folder_name)
                            }
                                on:contextmenu|preventDefault={() => toggleItemSelection({id: folder.folder_id, type: 'folder'})}
                        >
                            <div class="item-select-indicator">
                                {#if $isSelectionMode}
                                    <div class="checkbox">
                                        {#if $selectedItems.some(item => item.id === folder.folder_id && item.type === 'folder')}
                                            <i class="material-icons">check_box</i>
                                        {:else}
                                            <i class="material-icons">check_box_outline_blank</i>
                                        {/if}
                                    </div>
                                {/if}
                            </div>

                            <div class="item-icon folder-icon">
                                <i class="material-icons">folder</i>
                            </div>

                            <div class="item-details">
                                <div class="item-name">{folder.folder_name}</div>
                                <div class="item-meta">{formatDate(folder.created_at)}</div>
                            </div>
                        </div>
                    {/each}
                {/if}

                <!-- Затем отображаются файлы -->
                {#each filteredFiles as file}
                    <div
                            class="grid-item file-item"
                            class:selected={$selectedItems.some(item => item.id === file.file_id && item.type === 'file')}
                            on:click={() => toggleItemSelection({id: file.file_id, type: 'file'})}
                            on:dblclick={() => handleDownload(file.file_id, file.file_name)}
                            on:contextmenu|preventDefault={() => toggleItemSelection({id: file.file_id, type: 'file'})}
                    >
                        <div class="item-select-indicator">
                            {#if $isSelectionMode}
                                <div class="checkbox">
                                    {#if $selectedItems.some(item => item.id === file.file_id && item.type === 'file')}
                                        <i class="material-icons">check_box</i>
                                    {:else}
                                        <i class="material-icons">check_box_outline_blank</i>
                                    {/if}
                                </div>
                            {/if}
                        </div>

                        <div class="item-icon file-icon {getFileColorClass(file.file_name)}">
                            <i class="material-icons">{getFileIcon(file.file_name)}</i>
                        </div>

                        <div class="item-details">
                            <div class="item-name">{file.file_name}</div>
                            <div class="item-meta">
                                <span>{formatFileSize(file.file_size)}</span>
                                <span>•</span>
                                <span>{formatDate(file.created_at)}</span>
                            </div>
                        </div>

                        <div class="item-actions">
                            <button
                                    class="icon-button"
                                    title="Скачать"
                                    on:click|stopPropagation={() => handleDownload(file.file_id, file.file_name)}
                            >
                                <i class="material-icons">download</i>
                            </button>
                        </div>
                    </div>
                {/each}
            </div>
        {:else}
            <div class="list-view">
                <table class="files-table">
                    <thead>
                    <tr>
                        <th class="checkbox-column">
                            <div class="checkbox" on:click|stopPropagation={selectAllItems}>
                                {#if $selectedItems.length > 0 && $selectedItems.length === (filteredFiles.length + filteredFolders.length)}
                                    <i class="material-icons">check_box</i>
                                {:else}
                                    <i class="material-icons">check_box_outline_blank</i>
                                {/if}
                            </div>
                        </th>
                        <th class="name-column" on:click={() => setSortBy('name')}>
                            <div class="th-content">
                                <span>Имя</span>
                                {#if sortBy === 'name'}
                                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                                {/if}
                            </div>
                        </th>
                        <th class="date-column" on:click={() => setSortBy('date')}>
                            <div class="th-content">
                                <span>Дата создания</span>
                                {#if sortBy === 'date'}
                                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                                {/if}
                            </div>
                        </th>
                        <th class="size-column" on:click={() => setSortBy('size')}>
                            <div class="th-content">
                                <span>Размер</span>
                                {#if sortBy === 'size'}
                                    <i class="material-icons">{sortOrder === 'asc' ? 'arrow_upward' : 'arrow_downward'}</i>
                                {/if}
                            </div>
                        </th>
                        <th class="actions-column"></th>
                    </tr>
                    </thead>
                    <tbody>
                    {#if !searchQuery}
                        <!-- Папки всегда отображаются первыми -->
                        {#each filteredFolders as folder}
                            <tr
                                    class:selected={$selectedItems.some(item => item.id === folder.folder_id && item.type === 'folder')}
                                    on:click={() => $isSelectionMode
                                        ? toggleItemSelection({id: folder.folder_id, type: 'folder'})
                                        : navigateToFolder(folder.folder_id, folder.folder_name)
                                    }
                            >
                                <td class="checkbox-column">
                                    <div class="checkbox"
                                         on:click|stopPropagation={() => toggleItemSelection({id: folder.folder_id, type: 'folder'})}>
                                        {#if $selectedItems.some(item => item.id === folder.folder_id && item.type === 'folder')}
                                            <i class="material-icons">check_box</i>
                                        {:else}
                                            <i class="material-icons">check_box_outline_blank</i>
                                        {/if}
                                    </div>
                                </td>
                                <td class="name-column">
                                    <div class="item-name-cell">
                                        <i class="material-icons folder-icon">folder</i>
                                        <span>{folder.folder_name}</span>
                                    </div>
                                </td>
                                <td class="date-column">{formatDate(folder.created_at)}</td>
                                <td class="size-column">—</td>
                                <td class="actions-column">
                                    <div class="actions-container">
                                        <button
                                                class="icon-button"
                                                title="Открыть"
                                                on:click|stopPropagation={() => navigateToFolder(folder.folder_id, folder.folder_name)}
                                        >
                                            <i class="material-icons">folder_open</i>
                                        </button>
                                    </div>
                                </td>
                            </tr>
                        {/each}
                    {/if}

                    <!-- Затем отображаются файлы -->
                    {#each filteredFiles as file}
                        <tr
                                class:selected={$selectedItems.some(item => item.id === file.file_id && item.type === 'file')}
                                on:click={() => toggleItemSelection({id: file.file_id, type: 'file'})}
                        >
                            <td class="checkbox-column">
                                <div class="checkbox"
                                     on:click|stopPropagation={() => toggleItemSelection({id: file.file_id, type: 'file'})}>
                                    {#if $selectedItems.some(item => item.id === file.file_id && item.type === 'file')}
                                        <i class="material-icons">check_box</i>
                                    {:else}
                                        <i class="material-icons">check_box_outline_blank</i>
                                    {/if}
                                </div>
                            </td>
                            <td class="name-column">
                                <div class="item-name-cell">
                                    <i class="material-icons {getFileColorClass(file.file_name)}">{getFileIcon(file.file_name)}</i>
                                    <span>{file.file_name}</span>
                                </div>
                            </td>
                            <td class="date-column">{formatDate(file.created_at)}</td>
                            <td class="size-column">{formatFileSize(file.file_size)}</td>
                            <td class="actions-column">
                                <div class="actions-container">
                                    <button
                                            class="icon-button"
                                            title="Скачать"
                                            on:click|stopPropagation={() => handleDownload(file.file_id, file.file_name)}
                                    >
                                        <i class="material-icons">download</i>
                                    </button>
                                </div>
                            </td>
                        </tr>
                    {/each}
                    </tbody>
                </table>
            </div>
        {/if}
    {/if}
</div>

<!-- Модальное окно создания папки -->
{#if $showFolderModal}
    <div class="modal-overlay" on:click|self={() => $showFolderModal = false}>
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Создать новую папку</h3>
                <button class="close-button" on:click={() => $showFolderModal = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                <div class="input-group">
                    <label for="folder-name">Название папки</label>
                    <input
                            id="folder-name"
                            type="text"
                            bind:value={newFolderName}
                            placeholder="Введите название папки"
                            on:keydown={e => e.key === 'Enter' && handleCreateFolder()}
                    />
                </div>
            </div>
            <div class="modal-footer">
                <button class="secondary-button" on:click={() => $showFolderModal = false}>
                    Отмена
                </button>
                <button
                        class="primary-button"
                        disabled={!newFolderName.trim()}
                        on:click={handleCreateFolder}
                >
                    Создать
                </button>
            </div>
        </div>
    </div>
{/if}

<!-- Модальное окно подтверждения удаления -->
{#if $showDeleteConfirm}
    <div class="modal-overlay" on:click|self={() => $showDeleteConfirm = false}>
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Подтверждение удаления</h3>
                <button class="close-button" on:click={() => $showDeleteConfirm = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                <p>Вы уверены, что хотите удалить выбранные элементы ({$selectedItems.length})?</p>
                <p class="warning-text">Это действие нельзя отменить.</p>
            </div>
            <div class="modal-footer">
                <button class="secondary-button" on:click={() => $showDeleteConfirm = false}>
                    Отмена
                </button>
                <button class="danger-button" on:click={handleDelete}>
                    Удалить
                </button>
            </div>
        </div>
    </div>
{/if}

<!-- Модальное окно перемещения файлов -->
{#if showMoveModal}
    <div class="modal-overlay" on:click|self={() => showMoveModal = false}>
        <div class="modal-dialog">
            <div class="modal-header">
                <h3>Переместить элементы</h3>
                <button class="close-button" on:click={() => showMoveModal = false}>
                    <i class="material-icons">close</i>
                </button>
            </div>
            <div class="modal-body">
                <p>Выберите целевую папку для перемещения {$selectedItems.length} элементов:</p>

                <div class="folder-list">
                    <div
                            class="folder-list-item"
                            class:selected={targetMoveFolder === null}
                            on:click={() => selectTargetFolder(null)}
                    >
                        <i class="material-icons">cloud</i>
                        <span>Корневая директория</span>
                    </div>

                    {#each get(folders) as folder}
                        <div
                                class="folder-list-item"
                                class:selected={targetMoveFolder && targetMoveFolder.folder_id === folder.folder_id}
                                on:click={() => selectTargetFolder(folder)}
                        >
                            <i class="material-icons">folder</i>
                            <span>{folder.folder_name}</span>
                        </div>
                    {/each}
                </div>
            </div>
            <div class="modal-footer">
                <button class="secondary-button" on:click={() => showMoveModal = false}>
                    Отмена
                </button>
                <button
                        class="primary-button"
                        on:click={handleMoveItems}
                >
                    Переместить
                </button>
            </div>
        </div>
    </div>
{/if}

<!-- Оверлей загрузки -->
{#if $loading}
    <div class="loading-overlay">
        <div class="loader"></div>
    </div>
{/if}

<style>
    /* Подключаем Material Icons */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Основные стили */
    :global(.file-manager-container) {
        font-family: 'Roboto', sans-serif;
        color: #333;
        display: flex;
        flex-direction: column;
        height: 100%;
    }

    /* Верхняя панель */
    .file-manager-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 8px 16px;
        border-bottom: 1px solid #e0e0e0;
        background-color: #f5f5f5;
    }

    .breadcrumb-container {
        display: flex;
        align-items: center;
    }

    .breadcrumb {
        display: flex;
        align-items: center;
        flex-wrap: wrap;
    }

    .breadcrumb-item {
        display: inline-flex;
        align-items: center;
        font-size: 14px;
        color: #5f6368;
        cursor: pointer;
        padding: 4px 8px;
        border-radius: 4px;
    }

    .breadcrumb-item:hover {
        background-color: rgba(0, 0, 0, 0.05);
    }

    .breadcrumb-item.active {
        color: #1a73e8;
        font-weight: 500;
    }

    .breadcrumb-item i {
        margin-right: 4px;
        font-size: 18px;
    }

    .breadcrumb-separator {
        color: #5f6368;
        margin: 0 2px;
    }

    .search-container {
        flex: 0 1 400px;
    }

    .search-input-wrapper {
        position: relative;
        display: flex;
        align-items: center;
    }

    .search-input-wrapper i {
        position: absolute;
        left: 10px;
        color: #5f6368;
    }

    .search-input-wrapper input {
        width: 100%;
        padding: 8px 36px;
        border: 1px solid #dadce0;
        border-radius: 24px;
        font-size: 14px;
        transition: all 0.2s;
    }

    .search-input-wrapper input:focus {
        outline: none;
        border-color: #1a73e8;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
    }

    .clear-search-button {
        position: absolute;
        right: 10px;
        background: transparent;
        border: none;
        cursor: pointer;
        color: #5f6368;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 4px;
    }

    .clear-search-button:hover {
        color: #1a73e8;
    }

    /* Панель инструментов */
    .toolbar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 8px 16px;
        border-bottom: 1px solid #e0e0e0;
        background-color: white;
    }

    .left-actions, .right-actions {
        display: flex;
        align-items: center;
    }

    .primary-actions {
        display: flex;
        align-items: center;
    }

    .upload-button {
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 8px 16px;
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 4px;
        font-size: 14px;
        cursor: pointer;
        margin-right: 8px;
        transition: all 0.2s;
    }

    .upload-button:hover {
        background-color: #1765cc;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .upload-button i {
        margin-right: 8px;
    }

    .icon-button {
        width: 36px;
        height: 36px;
        display: flex;
        align-items: center;
        justify-content: center;
        background-color: transparent;
        border: none;
        border-radius: 50%;
        cursor: pointer;
        color: #5f6368;
        transition: all 0.2s;
    }

    .icon-button:hover {
        background-color: rgba(0, 0, 0, 0.05);
    }

    .icon-button:disabled {
        opacity: 0.5;
        cursor: default;
    }

    .icon-button.active {
        color: #1a73e8;
        background-color: rgba(26, 115, 232, 0.1);
    }

    .sort-options, .view-options {
        display: flex;
        align-items: center;
    }

    .sort-options {
        margin-left: 16px;
    }

    .sort-button {
        display: flex;
        align-items: center;
        padding: 6px 12px;
        background-color: transparent;
        border: none;
        border-radius: 4px;
        font-size: 14px;
        cursor: pointer;
        color: #5f6368;
    }

    .sort-button:hover {
        background-color: rgba(0, 0, 0, 0.05);
    }

    .sort-button.active {
        color: #1a73e8;
        font-weight: 500;
    }

    .sort-button i {
        margin-left: 4px;
        font-size: 16px;
    }

    .selection-info {
        display: flex;
        align-items: center;
        font-size: 14px;
        margin-right: 16px;
    }

    .selection-info span {
        margin-right: 8px;
    }

    .text-button {
        background-color: transparent;
        border: none;
        color: #1a73e8;
        cursor: pointer;
        font-size: 14px;
        padding: 4px 8px;
        border-radius: 4px;
    }

    .text-button:hover {
        background-color: rgba(26, 115, 232, 0.1);
    }

    .selection-actions {
        display: flex;
        align-items: center;
    }

    /* Основной контент */
    .file-manager-content {
        flex: 1;
        padding: 16px;
        overflow-y: auto;
        background-color: white;
    }

    /* Режим отображения: сетка */
    .grid-view {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
        gap: 16px;
    }

    .grid-item {
        position: relative;
        border: 1px solid #e0e0e0;
        border-radius: 8px;
        padding: 12px;
        cursor: pointer;
        transition: all 0.2s;
        display: flex;
        flex-direction: column;
        align-items: center;
    }

    .grid-item:hover {
        background-color: #f8f9fa;
        border-color: #dadce0;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .grid-item.selected {
        background-color: #e8f0fe;
        border-color: #1a73e8;
    }

    .item-select-indicator {
        position: absolute;
        top: 8px;
        left: 8px;
        z-index: 2;
    }

    .checkbox {
        cursor: pointer;
        color: #5f6368;
    }

    .checkbox:hover {
        color: #1a73e8;
    }

    .selected .checkbox {
        color: #1a73e8;
    }

    .item-icon {
        width: 48px;
        height: 48px;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-bottom: 8px;
        border-radius: 8px;
    }

    .folder-icon {
        color: #ffc107;
        background-color: rgba(255, 193, 7, 0.1);
    }

    .folder-icon i {
        font-size: 36px;
    }

    .file-icon {
        background-color: rgba(33, 150, 243, 0.1);
        color: #2196f3;
    }

    .file-icon i {
        font-size: 32px;
    }

    .file-icon-pdf {
        color: #ea4335;
        background-color: rgba(234, 67, 53, 0.1);
    }

    .file-icon-doc {
        color: #4285f4;
        background-color: rgba(66, 133, 244, 0.1);
    }

    .file-icon-sheet {
        color: #0f9d58;
        background-color: rgba(15, 157, 88, 0.1);
    }

    .file-icon-slide {
        color: #ff9800;
        background-color: rgba(255, 152, 0, 0.1);
    }

    .file-icon-image {
        color: #673ab7;
        background-color: rgba(103, 58, 183, 0.1);
    }

    .file-icon-audio {
        color: #9c27b0;
        background-color: rgba(156, 39, 176, 0.1);
    }

    .file-icon-video {
        color: #f44336;
        background-color: rgba(244, 67, 54, 0.1);
    }

    .file-icon-archive {
        color: #795548;
        background-color: rgba(121, 85, 72, 0.1);
    }

    .file-icon-text {
        color: #607d8b;
        background-color: rgba(96, 125, 139, 0.1);
    }

    .file-icon-code {
        color: #00bcd4;
        background-color: rgba(0, 188, 212, 0.1);
    }

    .file-icon-default {
        color: #2196f3;
        background-color: rgba(33, 150, 243, 0.1);
    }

    .item-details {
        text-align: center;
        width: 100%;
    }

    .item-name {
        font-size: 14px;
        font-weight: 500;
        margin-bottom: 4px;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    .item-meta {
        font-size: 12px;
        color: #5f6368;
    }

    .item-actions {
        opacity: 0;
        position: absolute;
        top: 8px;
        right: 8px;
        transition: opacity 0.2s;
    }

    .grid-item:hover .item-actions {
        opacity: 1;
    }

    /* Режим отображения: список */
    .list-view {
        width: 100%;
    }

    .files-table {
        width: 100%;
        border-collapse: collapse;
    }

    .files-table th {
        text-align: left;
        padding: 12px 16px;
        font-weight: 500;
        color: #5f6368;
        border-bottom: 1px solid #e0e0e0;
        user-select: none;
    }

    .files-table th:hover {
        background-color: #f8f9fa;
        cursor: pointer;
    }

    .th-content {
        display: flex;
        align-items: center;
    }

    .th-content i {
        margin-left: 4px;
        font-size: 16px;
    }

    .files-table td {
        padding: 12px 16px;
        border-bottom: 1px solid #e0e0e0;
    }

    .files-table tr {
        transition: background-color 0.2s;
    }

    .files-table tr:hover {
        background-color: #f8f9fa;
    }

    .files-table tr.selected {
        background-color: #e8f0fe;
    }

    .checkbox-column {
        width: 48px;
    }

    .name-column {
        min-width: 300px;
    }

    .date-column {
        width: 180px;
    }

    .size-column {
        width: 100px;
    }

    .actions-column {
        width: 100px;
    }

    .item-name-cell {
        display: flex;
        align-items: center;
    }

    .item-name-cell i {
        margin-right: 12px;
        font-size: 24px;
    }

    .actions-container {
        display: flex;
        align-items: center;
        justify-content: flex-end;
    }

    /* Пустое состояние */
    .empty-state {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 64px 0;
        text-align: center;
    }

    .empty-state i {
        font-size: 64px;
        color: #dadce0;
        margin-bottom: 16px;
    }

    .empty-state h3 {
        font-size: 20px;
        margin: 0 0 8px;
        color: #3c4043;
    }

    .empty-state p {
        font-size: 14px;
        margin: 0 0 24px;
        color: #5f6368;
    }

    .empty-actions {
        display: flex;
        gap: 16px;
    }

    /* Загрузка */
    .loading-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 64px 0;
    }

    .loader {
        border: 3px solid #f3f3f3;
        border-radius: 50%;
        border-top: 3px solid #1a73e8;
        width: 30px;
        height: 30px;
        animation: spin 1s linear infinite;
        margin-bottom: 16px;
    }

    @keyframes spin {
        0% {
            transform: rotate(0deg);
        }
        100% {
            transform: rotate(360deg);
        }
    }

    .loading-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(255, 255, 255, 0.8);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 1000;
    }

    /* Модальные окна */
    .modal-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0, 0, 0, 0.5);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 1000;
    }

    .modal-dialog {
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
        width: 400px;
        max-width: 90%;
        overflow: hidden;
    }

    .modal-header {
        padding: 16px 20px;
        border-bottom: 1px solid #e0e0e0;
        display: flex;
        justify-content: space-between;
        align-items: center;
    }

    .modal-header h3 {
        margin: 0;
        font-size: 18px;
        font-weight: 500;
    }

    .close-button {
        background: transparent;
        border: none;
        color: #5f6368;
        cursor: pointer;
        padding: 5px;
    }

    .modal-body {
        padding: 20px;
    }

    .modal-footer {
        padding: 16px 20px;
        border-top: 1px solid #e0e0e0;
        display: flex;
        justify-content: flex-end;
        gap: 10px;
    }

    .warning-text {
        color: #ea4335;
        font-weight: 500;
    }

    .input-group {
        margin-bottom: 16px;
    }

    .input-group label {
        display: block;
        margin-bottom: 8px;
        font-size: 14px;
        color: #5f6368;
    }

    .input-group input {
        width: 100%;
        padding: 8px 12px;
        border: 1px solid #dadce0;
        border-radius: 4px;
        font-size: 14px;
        transition: border-color 0.2s;
    }

    .input-group input:focus {
        outline: none;
        border-color: #1a73e8;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
    }

    /* Кнопки */
    .primary-button {
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 8px 16px;
        font-size: 14px;
        cursor: pointer;
        transition: all 0.2s;
    }

    .primary-button:hover {
        background-color: #1765cc;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .primary-button:disabled {
        background-color: #dadce0;
        color: #5f6368;
        cursor: not-allowed;
        box-shadow: none;
    }

    .secondary-button {
        background-color: white;
        color: #1a73e8;
        border: 1px solid #dadce0;
        border-radius: 4px;
        padding: 8px 16px;
        font-size: 14px;
        cursor: pointer;
        transition: all 0.2s;
    }

    .secondary-button:hover {
        background-color: #f8f9fa;
        border-color: #dadce0;
    }

    .danger-button {
        background-color: #ea4335;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 8px 16px;
        font-size: 14px;
        cursor: pointer;
        transition: all 0.2s;
    }

    .danger-button:hover {
        background-color: #d93025;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    /* Сообщения */
    .message {
        display: flex;
        align-items: center;
        padding: 12px 16px;
        margin-bottom: 16px;
        border-radius: 4px;
        font-size: 14px;
    }

    .message i {
        margin-right: 12px;
    }

    .message span {
        flex-grow: 1;
    }

    .message button {
        background: transparent;
        border: none;
        cursor: pointer;
        color: inherit;
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .error-message {
        background-color: #fdeded;
        color: #ea4335;
        border-left: 4px solid #ea4335;
    }

    .success-message {
        background-color: #e6f4ea;
        color: #1e8e3e;
        border-left: 4px solid #1e8e3e;
    }

    /* Список папок для модального окна перемещения */
    .folder-list {
        max-height: 300px;
        overflow-y: auto;
        border: 1px solid #dadce0;
        border-radius: 4px;
        margin-top: 12px;
    }

    .folder-list-item {
        display: flex;
        align-items: center;
        padding: 10px 16px;
        border-bottom: 1px solid #e0e0e0;
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .folder-list-item:last-child {
        border-bottom: none;
    }

    .folder-list-item:hover {
        background-color: #f8f9fa;
    }

    .folder-list-item.selected {
        background-color: #e8f0fe;
    }

    .folder-list-item i {
        margin-right: 12px;
        color: #ffc107;
    }

    .folder-list-item:first-child i {
        color: #1a73e8;
    }

    /* Адаптивность для мобильных устройств */
    @media (max-width: 768px) {
        .file-manager-header {
            flex-direction: column;
            align-items: flex-start;
        }

        .search-container {
            width: 100%;
            margin-top: 8px;
        }

        .toolbar {
            flex-wrap: wrap;
        }

        .right-actions {
            margin-top: 8px;
            width: 100%;
            justify-content: space-between;
        }

        .grid-view {
            grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
        }

        .files-table .date-column,
        .files-table .size-column {
            display: none;
        }
    }
</style>